/*
    FreeBASIC NuttX/RP2350-PiZero DVI smoke app
    ------------------------------------------------

    File: fbdvi_solid.c

    Purpose:

        Start a minimal DVI signal on the Waveshare RP2350-PiZero HDMI/DVI
        connector.

    Responsibilities:

        - configure the board's PIO0 based DVI serializer pins
        - configure the PWM differential pixel clock pins
        - allocate DMA channels through NuttX and build scanline DMA lists
        - output a solid active field using 640x480 style DVI timing

    This file intentionally does NOT contain:

        - a FreeBASIC framebuffer scanout path
        - palette expansion or gfxlib dirty-line handling
        - HDMI audio, EDID, or hotplug handling
        - a general RP23xx display driver

    Platform notes:

        The Waveshare RP2350-PiZero routes the DVI pairs to GPIO32, GPIO34,
        GPIO36, and GPIO38.  Those pins are outside the RP2350 HSTX pin group,
        so this smoke app uses the same PIO serializer route as the board
        vendor sample instead of the RP2350 HSTX example.
*/

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef FB_NUTTX_QEMU_DVI_SOLID_MODEL

#ifndef FAR
#  define FAR
#endif

#define FBDVI_PIO 0
#define FBDVI_PIO_GPIOBASE 16

#define FBDVI_LANE_COUNT 3
#define FBDVI_SYNC_LANE 0

#define FBDVI_SYMBOLS_PER_WORD 2
#define FBDVI_SYNC_LANE_CHUNKS 4
#define FBDVI_NOSYNC_LANE_CHUNKS 2

#define FBDVI_GPIO_BLUE 36
#define FBDVI_GPIO_GREEN 34
#define FBDVI_GPIO_RED 32
#define FBDVI_GPIO_CLOCK 38

#define FBDVI_PWM_CLOCK_SLICE 11
#define FBDVI_DMA_IRQ_PRIORITY 0x40u
#define FBDVI_DMA_WAIT_GUARD 10000u
#define FBDVI_HAZARD3_MEIPRA_CSR "0xbe3"

struct fbdvi_timing
{
  bool h_sync_polarity;
  uint16_t h_front_porch;
  uint16_t h_sync_width;
  uint16_t h_back_porch;
  uint16_t h_active_pixels;

  bool v_sync_polarity;
  uint16_t v_front_porch;
  uint16_t v_sync_width;
  uint16_t v_back_porch;
  uint16_t v_active_lines;
};

static const struct fbdvi_timing g_fbdvi_timing_640x480 =
{
  false,
  16,
  96,
  48,
  640,

  false,
  10,
  2,
  33,
  480
};

static const uint32_t g_fbdvi_ctrl_symbols[4] =
{
  0x000d5354,
  0x0002acab,
  0x00055154,
  0x000aaeab
};

static const uint32_t g_fbdvi_active_solid_tmds[FBDVI_LANE_COUNT] =
{
  0x0007fd00,
  0x0007fd00,
  0x000bfa01
};

static int fbdvi_model_check_timing(void)
{
  const struct fbdvi_timing *timing;
  unsigned int h_total;
  unsigned int v_total;

  timing = &g_fbdvi_timing_640x480;
  h_total = (unsigned int)timing->h_front_porch +
      (unsigned int)timing->h_sync_width +
      (unsigned int)timing->h_back_porch +
      (unsigned int)timing->h_active_pixels;
  v_total = (unsigned int)timing->v_front_porch +
      (unsigned int)timing->v_sync_width +
      (unsigned int)timing->v_back_porch +
      (unsigned int)timing->v_active_lines;

  if (h_total != 800 || v_total != 525)
    {
      return -1;
    }

  if (timing->h_active_pixels != 640 || timing->v_active_lines != 480)
    {
      return -1;
    }

  return 0;
}

static int fbdvi_model_check_symbols(void)
{
  if (g_fbdvi_ctrl_symbols[0] != 0x000d5354)
    {
      return -1;
    }

  if (g_fbdvi_ctrl_symbols[1] != 0x0002acab)
    {
      return -1;
    }

  if (g_fbdvi_ctrl_symbols[2] != 0x00055154)
    {
      return -1;
    }

  if (g_fbdvi_ctrl_symbols[3] != 0x000aaeab)
    {
      return -1;
    }

  if (g_fbdvi_active_solid_tmds[0] != 0x0007fd00 ||
      g_fbdvi_active_solid_tmds[1] != 0x0007fd00 ||
      g_fbdvi_active_solid_tmds[2] != 0x000bfa01)
    {
      return -1;
    }

  return 0;
}

int main(int argc, FAR char *argv[])
{
  (void)argc;
  (void)argv;

  if (fbdvi_model_check_timing() < 0)
    {
      printf("fbdvi: QEMU model timing check failed\n");
      return 1;
    }

  if (fbdvi_model_check_symbols() < 0)
    {
      printf("fbdvi: QEMU model symbol check failed\n");
      return 2;
    }

  printf("fbdvi: QEMU solid DVI model ok htotal=800 vtotal=525 active=640x480\n");
  printf("fbdvi: QEMU solid DVI model tmds=%08" PRIx32 ",%08" PRIx32 ",%08" PRIx32 "\n",
      g_fbdvi_active_solid_tmds[0],
      g_fbdvi_active_solid_tmds[1],
      g_fbdvi_active_solid_tmds[2]);
  printf("FB_NUTTX_QEMU_DVI_SOLID_MODEL_OK\n");

  return 0;
}

#else

#include <arch/chip/irq.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "hardware/rp23xx_dma.h"
#include "hardware/rp23xx_dreq.h"
#include "hardware/rp23xx_pads_bank0.h"
#include "hardware/rp23xx_pwm.h"
#include "rp23xx_dmac.h"
#include "rp23xx_gpio.h"
#include "rp23xx_pio.h"

#ifndef FAR
#  define FAR
#endif

#define FBDVI_PIO 0
#define FBDVI_PIO_GPIOBASE 16

#define FBDVI_LANE_COUNT 3
#define FBDVI_SYNC_LANE 0

#define FBDVI_SYMBOLS_PER_WORD 2
#define FBDVI_SYNC_LANE_CHUNKS 4
#define FBDVI_NOSYNC_LANE_CHUNKS 2

#define FBDVI_GPIO_BLUE 36
#define FBDVI_GPIO_GREEN 34
#define FBDVI_GPIO_RED 32
#define FBDVI_GPIO_CLOCK 38

#define FBDVI_PWM_CLOCK_SLICE 11
#define FBDVI_DMA_IRQ_PRIORITY 0x40u
#define FBDVI_DMA_WAIT_GUARD 10000u
#define FBDVI_HAZARD3_MEIPRA_CSR "0xbe3"

struct fbdvi_timing
{
  bool h_sync_polarity;
  uint16_t h_front_porch;
  uint16_t h_sync_width;
  uint16_t h_back_porch;
  uint16_t h_active_pixels;

  bool v_sync_polarity;
  uint16_t v_front_porch;
  uint16_t v_sync_width;
  uint16_t v_back_porch;
  uint16_t v_active_lines;
};

enum fbdvi_line_state
{
  FBDVI_STATE_FRONT_PORCH = 0,
  FBDVI_STATE_SYNC,
  FBDVI_STATE_BACK_PORCH,
  FBDVI_STATE_ACTIVE,
  FBDVI_STATE_COUNT
};

struct fbdvi_timing_state
{
  uint16_t v_ctr;
  enum fbdvi_line_state v_state;
};

struct fbdvi_dma_cb
{
  uint32_t ctrl_trig;
  uintptr_t read_addr;
  uintptr_t write_addr;
  uint32_t transfer_count;
};

struct fbdvi_lane
{
  DMA_HANDLE control;
  DMA_HANDLE data;
  unsigned int control_channel;
  unsigned int data_channel;
  uintptr_t tx_fifo;
  uint8_t dreq;
};

struct fbdvi_scanline_list
{
  struct fbdvi_dma_cb l0[FBDVI_SYNC_LANE_CHUNKS];
  struct fbdvi_dma_cb l1[FBDVI_NOSYNC_LANE_CHUNKS];
  struct fbdvi_dma_cb l2[FBDVI_NOSYNC_LANE_CHUNKS];
};

static const uint16_t g_fbdvi_serialiser_program[2] =
{
  0x78a1,
  0x74a1
};

static const struct fbdvi_timing g_fbdvi_timing_640x480 =
{
  false,
  16,
  96,
  48,
  640,

  false,
  10,
  2,
  33,
  480
};

/*
 * The DMA engine reads these words directly during scanout. Keep them in
 * writable static storage so the NuttX XIP image does not ask the DMA engine
 * to stream timing-critical DVI data from flash.
 */
static uint32_t g_fbdvi_ctrl_symbols[4] =
{
  0x000d5354,
  0x0002acab,
  0x00055154,
  0x000aaeab
};

static uint32_t g_fbdvi_active_solid_tmds[FBDVI_LANE_COUNT] =
{
  0x0007fd00,
  0x0007fd00,
  0x000bfa01
};

static struct fbdvi_lane g_fbdvi_lanes[FBDVI_LANE_COUNT];
static struct fbdvi_timing_state g_fbdvi_timing_state;
static struct fbdvi_scanline_list g_fbdvi_vblank_sync;
static struct fbdvi_scanline_list g_fbdvi_vblank_nosync;
static struct fbdvi_scanline_list g_fbdvi_active_solid;
static volatile uint32_t g_fbdvi_irq_count;
static volatile uint32_t g_fbdvi_wait_timeout_count;
static volatile uint32_t g_fbdvi_wait_timeout_lane;
static volatile uint32_t g_fbdvi_last_dma_intr;
static volatile bool g_fbdvi_faulted;
static bool g_fbdvi_started;

static unsigned int fbdvi_dma_channel(DMA_HANDLE handle)
{
  uintptr_t dma_register;

  dma_register = rp23xx_dma_register(handle, RP23XX_DMA_READ_ADDR_OFFSET);

  return (unsigned int)((dma_register - RP23XX_DMA_BASE) / 0x40u);
}

static struct fbdvi_dma_cb *fbdvi_lane_from_list(
    struct fbdvi_scanline_list *list, int lane)
{
  if (lane == 0)
    {
      return list->l0;
    }

  if (lane == 1)
    {
      return list->l1;
    }

  return list->l2;
}

static const uint32_t *fbdvi_ctrl_symbol(bool vsync, bool hsync)
{
  return &g_fbdvi_ctrl_symbols[(vsync ? 2 : 0) | (hsync ? 1 : 0)];
}

static void fbdvi_timing_state_advance(void)
{
  const struct fbdvi_timing *timing;
  uint16_t limit;

  timing = &g_fbdvi_timing_640x480;
  g_fbdvi_timing_state.v_ctr++;

  switch (g_fbdvi_timing_state.v_state)
    {
    case FBDVI_STATE_FRONT_PORCH:
      limit = timing->v_front_porch;
      break;

    case FBDVI_STATE_SYNC:
      limit = timing->v_sync_width;
      break;

    case FBDVI_STATE_BACK_PORCH:
      limit = timing->v_back_porch;
      break;

    default:
      limit = timing->v_active_lines;
      break;
    }

  if (g_fbdvi_timing_state.v_ctr < limit)
    {
      return;
    }

  g_fbdvi_timing_state.v_ctr = 0;
  g_fbdvi_timing_state.v_state =
      (enum fbdvi_line_state)((g_fbdvi_timing_state.v_state + 1) %
      FBDVI_STATE_COUNT);
}

static uint32_t fbdvi_data_ctrl(const struct fbdvi_lane *lane,
    unsigned int read_ring, bool irq_on_finish)
{
  uint32_t ctrl;

  ctrl = RP23XX_DMA_CTRL_TRIG_READ_ERROR |
      RP23XX_DMA_CTRL_TRIG_WRITE_ERROR |
      RP23XX_DMA_CTRL_TRIG_EN |
      RP23XX_DMA_CTRL_TRIG_INCR_READ |
      RP23XX_DMA_CTRL_TRIG_HIGH_PRIORITY |
      ((uint32_t)lane->dreq << RP23XX_DMA_CTRL_TRIG_TREQ_SEL_SHIFT) |
      ((uint32_t)lane->control_channel <<
      RP23XX_DMA_CTRL_TRIG_CHAIN_TO_SHIFT) |
      (RP23XX_DMA_SIZE_WORD << RP23XX_DMA_CTRL_TRIG_DATA_SIZE_SHIFT);

  if (read_ring != 0)
    {
      ctrl |= read_ring << RP23XX_DMA_CTRL_TRIG_RING_SIZE_SHIFT;
    }

  if (!irq_on_finish)
    {
      ctrl |= RP23XX_DMA_CTRL_TRIG_IRQ_QUIET;
    }

  return ctrl;
}

static void fbdvi_set_data_cb(struct fbdvi_dma_cb *cb,
    const struct fbdvi_lane *lane, const uint32_t *read_addr,
    unsigned int transfer_count, unsigned int read_ring,
    bool irq_on_finish)
{
  cb->read_addr = (uintptr_t)read_addr;
  cb->write_addr = lane->tx_fifo;
  cb->transfer_count = transfer_count;
  cb->ctrl_trig = fbdvi_data_ctrl(lane, read_ring, irq_on_finish);
}

static void fbdvi_setup_vblank(bool vsync_asserted,
    struct fbdvi_scanline_list *list)
{
  const struct fbdvi_timing *timing;
  const uint32_t *hsync_off;
  const uint32_t *hsync_on;
  const uint32_t *no_sync;
  struct fbdvi_dma_cb *lane_list;
  bool vsync;
  int lane;

  timing = &g_fbdvi_timing_640x480;
  vsync = timing->v_sync_polarity == vsync_asserted;
  hsync_off = fbdvi_ctrl_symbol(vsync, !timing->h_sync_polarity);
  hsync_on = fbdvi_ctrl_symbol(vsync, timing->h_sync_polarity);
  no_sync = fbdvi_ctrl_symbol(false, false);

  lane_list = fbdvi_lane_from_list(list, FBDVI_SYNC_LANE);
  fbdvi_set_data_cb(&lane_list[0], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_off, timing->h_front_porch / FBDVI_SYMBOLS_PER_WORD, 2, false);
  fbdvi_set_data_cb(&lane_list[1], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_on, timing->h_sync_width / FBDVI_SYMBOLS_PER_WORD, 2, false);
  fbdvi_set_data_cb(&lane_list[2], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_off, timing->h_back_porch / FBDVI_SYMBOLS_PER_WORD, 2, true);
  fbdvi_set_data_cb(&lane_list[3], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_off, timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD, 2, false);

  for (lane = 1; lane < FBDVI_LANE_COUNT; lane++)
    {
      lane_list = fbdvi_lane_from_list(list, lane);
      fbdvi_set_data_cb(&lane_list[0], &g_fbdvi_lanes[lane],
          no_sync,
          (timing->h_front_porch + timing->h_sync_width +
          timing->h_back_porch) / FBDVI_SYMBOLS_PER_WORD,
          2, false);
      fbdvi_set_data_cb(&lane_list[1], &g_fbdvi_lanes[lane],
          no_sync, timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD,
          2, false);
    }
}

static void fbdvi_setup_active_solid(struct fbdvi_scanline_list *list)
{
  const struct fbdvi_timing *timing;
  const uint32_t *hsync_off;
  const uint32_t *hsync_on;
  const uint32_t *no_sync;
  struct fbdvi_dma_cb *lane_list;
  int lane;

  timing = &g_fbdvi_timing_640x480;
  hsync_off = fbdvi_ctrl_symbol(!timing->v_sync_polarity,
      !timing->h_sync_polarity);
  hsync_on = fbdvi_ctrl_symbol(!timing->v_sync_polarity,
      timing->h_sync_polarity);
  no_sync = fbdvi_ctrl_symbol(false, false);

  lane_list = fbdvi_lane_from_list(list, FBDVI_SYNC_LANE);
  fbdvi_set_data_cb(&lane_list[0], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_off, timing->h_front_porch / FBDVI_SYMBOLS_PER_WORD, 2, false);
  fbdvi_set_data_cb(&lane_list[1], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_on, timing->h_sync_width / FBDVI_SYMBOLS_PER_WORD, 2, false);
  fbdvi_set_data_cb(&lane_list[2], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      hsync_off, timing->h_back_porch / FBDVI_SYMBOLS_PER_WORD, 2, true);
  fbdvi_set_data_cb(&lane_list[3], &g_fbdvi_lanes[FBDVI_SYNC_LANE],
      &g_fbdvi_active_solid_tmds[FBDVI_SYNC_LANE],
      timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD, 2, false);

  for (lane = 1; lane < FBDVI_LANE_COUNT; lane++)
    {
      lane_list = fbdvi_lane_from_list(list, lane);
      fbdvi_set_data_cb(&lane_list[0], &g_fbdvi_lanes[lane],
          no_sync,
          (timing->h_front_porch + timing->h_sync_width +
          timing->h_back_porch) / FBDVI_SYMBOLS_PER_WORD,
          2, false);
      fbdvi_set_data_cb(&lane_list[1], &g_fbdvi_lanes[lane],
          &g_fbdvi_active_solid_tmds[lane],
          timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD, 2, false);
    }
}

static uint32_t fbdvi_control_ctrl(unsigned int control_channel)
{
  return RP23XX_DMA_CTRL_TRIG_READ_ERROR |
      RP23XX_DMA_CTRL_TRIG_WRITE_ERROR |
      RP23XX_DMA_CTRL_TRIG_INCR_READ |
      RP23XX_DMA_CTRL_TRIG_INCR_WRITE |
      RP23XX_DMA_CTRL_TRIG_HIGH_PRIORITY |
      RP23XX_DMA_CTRL_TRIG_RING_SEL |
      (4u << RP23XX_DMA_CTRL_TRIG_RING_SIZE_SHIFT) |
      (RP23XX_DMA_SIZE_WORD << RP23XX_DMA_CTRL_TRIG_DATA_SIZE_SHIFT) |
      ((uint32_t)control_channel << RP23XX_DMA_CTRL_TRIG_CHAIN_TO_SHIFT) |
      ((uint32_t)RP23XX_DMA_DREQ_FORCE <<
      RP23XX_DMA_CTRL_TRIG_TREQ_SEL_SHIFT);
}

static void fbdvi_configure_control_channel(struct fbdvi_lane *lane,
    struct fbdvi_scanline_list *list)
{
  struct fbdvi_dma_cb *lane_list;

  lane_list = fbdvi_lane_from_list(list, (int)(lane - g_fbdvi_lanes));

  putreg32((uintptr_t)lane_list, RP23XX_DMA_READ_ADDR(lane->control_channel));
  /* NuttX's RP23xx DMA helper feeds control blocks through the AL1 alias.
   * The alias order is CTRL, READ_ADDR, WRITE_ADDR, TRANS_COUNT_TRIG.
   */

  putreg32(RP23XX_DMA_AL1_CTRL(lane->data_channel),
      RP23XX_DMA_WRITE_ADDR(lane->control_channel));
  putreg32(4, RP23XX_DMA_TRANS_COUNT(lane->control_channel));
  putreg32(fbdvi_control_ctrl(lane->control_channel),
      RP23XX_DMA_CTRL_TRIG(lane->control_channel));
}

static void fbdvi_enable_control_channel(struct fbdvi_lane *lane)
{
  /* CTRL_TRIG starts the channel as soon as the enable bit is written.
   * The AL1_CTRL alias updates the same control word without triggering it,
   * so the three lanes can still be started together below.
   */

  putreg32(fbdvi_control_ctrl(lane->control_channel) |
      RP23XX_DMA_CTRL_TRIG_EN,
      RP23XX_DMA_AL1_CTRL(lane->control_channel));
}

static void fbdvi_load_dma_list(struct fbdvi_scanline_list *list)
{
  struct fbdvi_lane *lane;
  int i;

  for (i = 0; i < FBDVI_LANE_COUNT; i++)
    {
      lane = &g_fbdvi_lanes[i];
      putreg32((uintptr_t)fbdvi_lane_from_list(list, i),
          RP23XX_DMA_READ_ADDR(lane->control_channel));
      putreg32(4, RP23XX_DMA_TRANS_COUNT(lane->control_channel));
    }
}

static void fbdvi_dump_cb(const char *name, struct fbdvi_scanline_list *list)
{
  struct fbdvi_dma_cb *lane_list;
  int lane;
  int count;
  int i;

  printf("fbdvi: cb %s base=%08" PRIxPTR "\n", name, (uintptr_t)list);

  for (lane = 0; lane < FBDVI_LANE_COUNT; lane++)
    {
      lane_list = fbdvi_lane_from_list(list, lane);
      count = lane == FBDVI_SYNC_LANE ?
          FBDVI_SYNC_LANE_CHUNKS : FBDVI_NOSYNC_LANE_CHUNKS;

      for (i = 0; i < count; i++)
        {
          printf("fbdvi: cb %s lane=%d block=%d rd=%08" PRIxPTR
              " wr=%08" PRIxPTR " cnt=%08" PRIx32
              " ctrl=%08" PRIx32 "\n",
              name, lane, i,
              lane_list[i].read_addr,
              lane_list[i].write_addr,
              lane_list[i].transfer_count,
              lane_list[i].ctrl_trig);
        }
    }
}

static void fbdvi_wait_for_active_blocks_loaded(void)
{
  const struct fbdvi_timing *timing;
  uint32_t expected_count;
  unsigned int guard;
  int i;

  timing = &g_fbdvi_timing_640x480;
  expected_count = timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD;

  /*
   * The scanline IRQ fires at the end of the sync lane's back porch block.
   * Before repointing the control channels at the next scanline, PicoDVI
   * waits until all three data channels have definitely loaded their active
   * block.  Without this guard, the IRQ can race the non-sync lanes and make
   * them skip or corrupt the active transfer, which quickly drains the PIO
   * FIFOs.
   */

  for (i = 0; i < FBDVI_LANE_COUNT; i++)
    {
      guard = FBDVI_DMA_WAIT_GUARD;

      while (getreg32(RP23XX_DMA_DBG_TCR(g_fbdvi_lanes[i].data_channel)) !=
          expected_count)
        {
          guard--;

          if (guard == 0)
            {
              g_fbdvi_wait_timeout_count++;
              g_fbdvi_wait_timeout_lane = (uint32_t)i;
              g_fbdvi_faulted = true;
              break;
            }
        }
    }
}

static int fbdvi_dma_irq(int irq, void *context, void *arg)
{
  struct fbdvi_scanline_list *next_list;
  uint32_t irq_bit;

  (void)irq;
  (void)context;
  (void)arg;

  irq_bit = 1u << g_fbdvi_lanes[FBDVI_SYNC_LANE].data_channel;
  g_fbdvi_irq_count++;
  g_fbdvi_last_dma_intr = getreg32(RP23XX_DMA_INTR);
  putreg32(irq_bit, RP23XX_DMA_INTS1);

  fbdvi_timing_state_advance();
  fbdvi_wait_for_active_blocks_loaded();

  if (g_fbdvi_faulted)
    {
      clrbits_reg32(irq_bit, RP23XX_DMA_INTE1);
      return 0;
    }

  switch (g_fbdvi_timing_state.v_state)
    {
    case FBDVI_STATE_SYNC:
      next_list = &g_fbdvi_vblank_sync;
      break;

    case FBDVI_STATE_ACTIVE:
      next_list = &g_fbdvi_active_solid;
      break;

    default:
      next_list = &g_fbdvi_vblank_nosync;
      break;
    }

  fbdvi_load_dma_list(next_list);

  return 0;
}

static void fbdvi_configure_pad(unsigned int gpio)
{
  uint32_t pad_bits;

  pad_bits = RP23XX_PADS_BANK0_GPIO_DRIVE_2MA;

  modbits_reg32(pad_bits,
      RP23XX_PADS_BANK0_GPIO_DRIVE_MASK |
      RP23XX_PADS_BANK0_GPIO_SLEWFAST |
      RP23XX_PADS_BANK0_GPIO_IE,
      RP23XX_PADS_BANK0_GPIO(gpio));
}

static void fbdvi_configure_tmds_gpio(unsigned int gpio)
{
  rp23xx_gpio_set_function(gpio, RP23XX_GPIO_FUNC_PIO0);
  rp23xx_gpio_set_function(gpio + 1, RP23XX_GPIO_FUNC_PIO0);
  fbdvi_configure_pad(gpio);
  fbdvi_configure_pad(gpio + 1);
}

static void fbdvi_configure_clock_gpio(void)
{
  rp23xx_gpio_set_function(FBDVI_GPIO_CLOCK, RP23XX_GPIO_FUNC_PWM);
  rp23xx_gpio_set_function(FBDVI_GPIO_CLOCK + 1, RP23XX_GPIO_FUNC_PWM);
  fbdvi_configure_pad(FBDVI_GPIO_CLOCK);
  fbdvi_configure_pad(FBDVI_GPIO_CLOCK + 1);
}

static void fbdvi_configure_pwm_clock(void)
{
  uint32_t channel_bit;

  channel_bit = 1u << FBDVI_PWM_CLOCK_SLICE;

  clrbits_reg32(channel_bit, RP23XX_RV_PWM_EN);
  putreg32(0, RP23XX_RV_PWM_CTR(FBDVI_PWM_CLOCK_SLICE));
  putreg32(1u << RP23XX_RV_PWM_DIV_INT_SHIFT,
      RP23XX_RV_PWM_DIV(FBDVI_PWM_CLOCK_SLICE));
  putreg32(9, RP23XX_RV_PWM_TOP(FBDVI_PWM_CLOCK_SLICE));
  putreg32((5u << RP23XX_RV_PWM_CC_A_SHIFT) |
      (5u << RP23XX_RV_PWM_CC_B_SHIFT),
      RP23XX_RV_PWM_CC(FBDVI_PWM_CLOCK_SLICE));
  putreg32(RP23XX_RV_PWM_CSR_A_INV,
      RP23XX_RV_PWM_CSR(FBDVI_PWM_CLOCK_SLICE));
}

static void fbdvi_enable_pwm_clock(void)
{
  setbits_reg32(1u << FBDVI_PWM_CLOCK_SLICE, RP23XX_RV_PWM_EN);
}

static void fbdvi_configure_pio_sm(unsigned int sm, unsigned int gpio)
{
  rp23xx_pio_sm_config config;
  uint32_t pin_mask;
  uint32_t pin_values;
  unsigned int virtual_pin;

  virtual_pin = gpio - FBDVI_PIO_GPIOBASE;
  pin_mask = 3u << virtual_pin;
  pin_values = 2u << virtual_pin;

  /* The RP2350-PiZero DVI pins live in the PIO GPIOBASE 16 window.  Use
   * virtual pin numbers here, matching PicoDVI's pio_sm_set_* calls after it
   * selects that GPIO base.
   */

  rp23xx_pio_sm_set_pins_with_mask(FBDVI_PIO, sm, pin_values, pin_mask);
  rp23xx_pio_sm_set_pindirs_with_mask(FBDVI_PIO, sm, pin_mask, pin_mask);

  config = rp23xx_pio_get_default_sm_config();
  rp23xx_sm_config_set_wrap(&config, 0, 1);
  rp23xx_sm_config_set_sideset(&config, 3, true, false);
  rp23xx_sm_config_set_sideset_pins(&config, virtual_pin);
  rp23xx_sm_config_set_out_shift(&config, true, true, 20);
  rp23xx_sm_config_set_fifo_join(&config, RP23XX_PIO_FIFO_JOIN_TX);
  rp23xx_pio_sm_init(FBDVI_PIO, sm, 0, &config);
}

static void fbdvi_configure_pio(void)
{
  int i;

  clrbits_reg32(RP23XX_PIO_CTRL_SM_ENABLE_MASK, RP23XX_PIO_CTRL(FBDVI_PIO));
  putreg32(FBDVI_PIO_GPIOBASE, RP23XX_PIO_GPIOBASE(FBDVI_PIO));

  for (i = 0; i < 2; i++)
    {
      putreg32(g_fbdvi_serialiser_program[i],
          RP23XX_PIO_INSTR_MEM(FBDVI_PIO, i));
    }

  fbdvi_configure_tmds_gpio(FBDVI_GPIO_BLUE);
  fbdvi_configure_tmds_gpio(FBDVI_GPIO_GREEN);
  fbdvi_configure_tmds_gpio(FBDVI_GPIO_RED);

  fbdvi_configure_pio_sm(0, FBDVI_GPIO_BLUE);
  fbdvi_configure_pio_sm(1, FBDVI_GPIO_GREEN);
  fbdvi_configure_pio_sm(2, FBDVI_GPIO_RED);
}

static int fbdvi_wait_for_fifos_full(void)
{
  uint32_t full_mask;
  unsigned int guard;

  full_mask = (1u << (16 + 0)) | (1u << (16 + 1)) | (1u << (16 + 2));
  guard = 1000000;

  while ((getreg32(RP23XX_PIO_FSTAT(FBDVI_PIO)) & full_mask) != full_mask)
    {
      guard--;

      if (guard == 0)
        {
          return -1;
        }
    }

  return 0;
}

static void fbdvi_enable_pio(void)
{
  uint32_t sm_mask;

  sm_mask = (1u << 0) | (1u << 1) | (1u << 2);

  /*
   * The three TMDS lanes must begin on the same PIO clock edge.  The Pico SDK
   * helper used by the vendor DVI sample restarts the selected clock dividers
   * and enables the state machines in one register write; use the NuttX helper
   * that provides the same hardware operation.
   */

  rp23xx_pio_enable_sm_mask_in_sync(FBDVI_PIO, sm_mask);
}

static int fbdvi_allocate_dma(void)
{
  int i;

  for (i = 0; i < FBDVI_LANE_COUNT; i++)
    {
      g_fbdvi_lanes[i].control = rp23xx_dmachannel();
      g_fbdvi_lanes[i].data = rp23xx_dmachannel();

      if (g_fbdvi_lanes[i].control == NULL || g_fbdvi_lanes[i].data == NULL)
        {
          return -1;
        }

      g_fbdvi_lanes[i].control_channel =
          fbdvi_dma_channel(g_fbdvi_lanes[i].control);
      g_fbdvi_lanes[i].data_channel = fbdvi_dma_channel(g_fbdvi_lanes[i].data);
      g_fbdvi_lanes[i].tx_fifo = RP23XX_PIO_TXF(FBDVI_PIO, i);
      g_fbdvi_lanes[i].dreq = (uint8_t)(RP23XX_DMA_DREQ_PIO0_TX0 + i);

      printf("fbdvi: lane %d control=%u data=%u dreq=%u fifo=%08" PRIxPTR
          "\n",
          i, g_fbdvi_lanes[i].control_channel,
          g_fbdvi_lanes[i].data_channel, g_fbdvi_lanes[i].dreq,
          g_fbdvi_lanes[i].tx_fifo);

      clrbits_reg32((1u << g_fbdvi_lanes[i].control_channel) |
          (1u << g_fbdvi_lanes[i].data_channel), RP23XX_DMA_INTE0);
      putreg32((1u << g_fbdvi_lanes[i].control_channel) |
          (1u << g_fbdvi_lanes[i].data_channel), RP23XX_DMA_INTS0);
    }

  return 0;
}

static void fbdvi_hazard3_irqarray_clear_meipra(uint32_t index,
    uint32_t data)
{
  uint32_t value;

  value = index | (data << 16);

  /* MEIPRA is Hazard3's external interrupt priority array. */

  __asm__ __volatile__("csrc " FBDVI_HAZARD3_MEIPRA_CSR ", %0" ::
      "rK"(value) : "memory");
}

static void fbdvi_hazard3_irqarray_set_meipra(uint32_t index,
    uint32_t data)
{
  uint32_t value;

  value = index | (data << 16);

  /* MEIPRA is Hazard3's external interrupt priority array. */

  __asm__ __volatile__("csrs " FBDVI_HAZARD3_MEIPRA_CSR ", %0" ::
      "rK"(value) : "memory");
}

static void fbdvi_route_dma_irq(void)
{
  uint32_t irq_bit;
  uint32_t hardware_priority;
  uint32_t priority_shift;
  uint32_t extirq;

  irq_bit = 1u << g_fbdvi_lanes[FBDVI_SYNC_LANE].data_channel;

  /*
   * PicoDVI gives its DMA IRQ a high priority so the scanline handler runs
   * during the active-video window. NuttX initializes RP2350 external IRQs
   * at one shared priority, which is too easy for USB console traffic to
   * disturb during bring-up.
   */

  extirq = RP23XX_DMA_IRQ_1 - RP23XX_IRQ_EXTINT;
  hardware_priority = ((FBDVI_DMA_IRQ_PRIORITY >> 4) ^ 0x0fu) & 0x0fu;
  priority_shift = 4u * (extirq % 4u);

  fbdvi_hazard3_irqarray_clear_meipra(extirq / 4u,
      0x0fu << priority_shift);
  fbdvi_hazard3_irqarray_set_meipra(extirq / 4u,
      hardware_priority << priority_shift);

  irq_attach(RP23XX_DMA_IRQ_1, fbdvi_dma_irq, NULL);
  putreg32(irq_bit, RP23XX_DMA_INTS1);
  setbits_reg32(irq_bit, RP23XX_DMA_INTE1);
  up_enable_irq(RP23XX_DMA_IRQ_1);
}

static void fbdvi_start_dma(void)
{
  int i;
  uint32_t channel_mask;

  fbdvi_configure_control_channel(&g_fbdvi_lanes[0], &g_fbdvi_vblank_nosync);
  fbdvi_configure_control_channel(&g_fbdvi_lanes[1], &g_fbdvi_vblank_nosync);
  fbdvi_configure_control_channel(&g_fbdvi_lanes[2], &g_fbdvi_vblank_nosync);

  channel_mask = 0;

  for (i = 0; i < FBDVI_LANE_COUNT; i++)
    {
      fbdvi_enable_control_channel(&g_fbdvi_lanes[i]);
      channel_mask |= 1u << g_fbdvi_lanes[i].control_channel;
    }

  /* MULTI_CHAN_TRIGGER starts enabled channels.  The control channels were
   * enabled through AL1_CTRL above so this write synchronizes the first load
   * for all three TMDS lanes.
   */

  putreg32(channel_mask, RP23XX_DMA_MULTI_CHAN_TRIGGER);
}

static int fbdvi_start(void)
{
  if (g_fbdvi_started)
    {
      return 0;
    }

  if (fbdvi_allocate_dma() < 0)
    {
      return -1;
    }

  fbdvi_configure_clock_gpio();
  fbdvi_configure_pwm_clock();
  fbdvi_configure_pio();

  fbdvi_setup_vblank(true, &g_fbdvi_vblank_sync);
  fbdvi_setup_vblank(false, &g_fbdvi_vblank_nosync);
  fbdvi_setup_active_solid(&g_fbdvi_active_solid);
  fbdvi_dump_cb("vblank_sync", &g_fbdvi_vblank_sync);
  fbdvi_dump_cb("vblank_nosync", &g_fbdvi_vblank_nosync);
  fbdvi_dump_cb("active", &g_fbdvi_active_solid);

  g_fbdvi_timing_state.v_ctr = 0;
  g_fbdvi_timing_state.v_state = FBDVI_STATE_FRONT_PORCH;

  fbdvi_route_dma_irq();
  fbdvi_start_dma();

  if (fbdvi_wait_for_fifos_full() < 0)
    {
      return -1;
    }

  fbdvi_enable_pio();
  fbdvi_enable_pwm_clock();

  g_fbdvi_started = true;

  return 0;
}

int main(int argc, FAR char *argv[])
{
  int ret;

  (void)argc;
  (void)argv;

  ret = fbdvi_start();

  if (ret < 0)
    {
      printf("fbdvi: failed to start DVI output\n");

      for (;;)
        {
          up_mdelay(1000);
        }
    }

  printf("fbdvi: solid DVI output started\n");

  for (;;)
    {
      printf("fbdvi: irq=%" PRIu32 " wait_timeout=%" PRIu32
          " wait_lane=%" PRIu32 " fault=%u state=%u ctr=%u"
          " intr=%08" PRIx32 " fstat=%08" PRIx32 "\n",
          g_fbdvi_irq_count, g_fbdvi_wait_timeout_count,
          g_fbdvi_wait_timeout_lane,
          g_fbdvi_faulted ? 1u : 0u,
          (unsigned int)g_fbdvi_timing_state.v_state,
          (unsigned int)g_fbdvi_timing_state.v_ctr,
          g_fbdvi_last_dma_intr,
          getreg32(RP23XX_PIO_FSTAT(FBDVI_PIO)));
      up_mdelay(1000);
    }
}

#endif

/* end of fbdvi_solid.c */
