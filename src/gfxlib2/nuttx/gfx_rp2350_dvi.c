/*
    FreeBASIC gfxlib2 NuttX RP2350-PiZero DVI scanout
    -------------------------------------------------

    File: gfx_rp2350_dvi.c

    Purpose:

        Provide the first board-specific DVI scanout path for the
        NuttX gfxlib2 driver on the Waveshare RP2350-PiZero.

    Responsibilities:

        - start the board's PIO/PWM/DMA DVI signal generator
        - translate the active 320x200 paletted gfxlib framebuffer into
          small TMDS scanline buffers
        - keep the scanout memory footprint below a full framebuffer copy

    This file intentionally does NOT contain:

        - generic gfxlib drawing commands
        - a NuttX /dev/fb video driver
        - USB keyboard handling
        - HDMI audio or EDID support

    Notes:

        The RP2350-PiZero exposes the DVI pairs on GPIO32, GPIO34, GPIO36,
        and GPIO38.  Those pins are outside the HSTX pin group, so this uses
        the PIO serializer route proven by the board smoke app.
*/

#include <nuttx/config.h>

#include "../fb_gfx.h"

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
#include <stdint.h>
#include <stdio.h>
#endif

#if defined(CONFIG_ARCH_CHIP_RP23XX_RV) && \
    defined(CONFIG_RP23XX_RV_PIZERO_DVI_CLOCK) && \
    defined(CONFIG_RP23XX_RV_DMAC)

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

#define FBDVI_FRAMEBUFFER_WIDTH 320
#define FBDVI_FRAMEBUFFER_HEIGHT 200
#define FBDVI_OUTPUT_TOP_BORDER 40
#define FBDVI_OUTPUT_SCALE_Y 2
#define FBDVI_ACTIVE_WORDS 320

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
    The DMA engine reads these symbols directly while the video signal is
    active. Keep them in .data instead of XIP/rodata; the PicoDVI examples
    solve the same constraint by building with PICO_COPY_TO_RAM.
*/
static uint32_t g_fbdvi_ctrl_symbols[4] =
{
    0x000d5354,
    0x0002acab,
    0x00055154,
    0x000aaeab
};

static const uint32_t g_fbdvi_tmds_pair_table[64] =
{
    0x0007fd00, 0x0040dfc, 0x0041df8, 0x0007ed04,
    0x0043df0, 0x0007cd0c, 0x0007dd08, 0x0042df4,
    0x0047de0, 0x00078d1c, 0x00079d18, 0x0046de4,
    0x0007bd10, 0x0044dec, 0x0045de8, 0x000afa41,
    0x004fdc0, 0x00070d3c, 0x00071d38, 0x004edc4,
    0x00073d30, 0x004cdcc, 0x004ddc8, 0x000a7a61,
    0x00077d20, 0x0048ddc, 0x0049dd8, 0x000a3a71,
    0x004bdd0, 0x000a1a79, 0x000a0a7d, 0x0009fa81,
    0x005fd80, 0x00060d7c, 0x00061d78, 0x005ed84,
    0x00063d70, 0x005cd8c, 0x005dd88, 0x000b7a21,
    0x00067d60, 0x0058d9c, 0x0059d98, 0x000b3a31,
    0x005bd90, 0x000b1a39, 0x000b0a3d, 0x0008fac1,
    0x006fd40, 0x0050dbc, 0x0051db8, 0x000bba11,
    0x0053db0, 0x000b9a19, 0x000b8a1d, 0x00087ae1,
    0x0057da0, 0x000bda09, 0x000bca0d, 0x00083af1,
    0x000bea05, 0x00081af9, 0x00080afd, 0x000bfa01
};

static struct fbdvi_lane g_fbdvi_lanes[FBDVI_LANE_COUNT];
static struct fbdvi_timing_state g_fbdvi_timing_state;
static struct fbdvi_scanline_list g_fbdvi_vblank_sync;
static struct fbdvi_scanline_list g_fbdvi_vblank_nosync;
static struct fbdvi_scanline_list g_fbdvi_active_scanline;
static uint32_t g_fbdvi_line_buffer[2][FBDVI_LANE_COUNT][FBDVI_ACTIVE_WORDS];
static uint32_t g_fbdvi_palette_words[256][FBDVI_LANE_COUNT];
static uint32_t g_fbdvi_black_words[FBDVI_LANE_COUNT];
static volatile int g_fbdvi_display_buffer;
static volatile int g_fbdvi_scanout_enabled;
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
        return list->l0;

    if (lane == 1)
        return list->l1;

    return list->l2;
}

static const uint32_t *fbdvi_ctrl_symbol(bool vsync, bool hsync)
{
    return &g_fbdvi_ctrl_symbols[(vsync ? 2 : 0) | (hsync ? 1 : 0)];
}

static uint32_t fbdvi_tmds_word(uint8_t value)
{
    /*
       This driver feeds a 320 pixel paletted framebuffer into 640 pixel DVI
       timing, so each source pixel becomes one word containing two TMDS
       symbols.  PicoDVI uses a 6-bit lookup for that doubled-pixel case: the
       two symbols are balanced as a pair and the encoded value is allowed to
       differ by one LSB.  That matches the standalone board smoke app and is
       a better fit here than running the full 8-bit TMDS encoder twice.
    */
    return g_fbdvi_tmds_pair_table[value >> 2];
}

static void fbdvi_set_palette_rgb(int index, uint8_t red, uint8_t green,
    uint8_t blue)
{
    if ((index < 0) || (index >= 256))
        return;

    g_fbdvi_palette_words[index][0] = fbdvi_tmds_word(blue);
    g_fbdvi_palette_words[index][1] = fbdvi_tmds_word(green);
    g_fbdvi_palette_words[index][2] = fbdvi_tmds_word(red);
}

static void fbdvi_init_palette(void)
{
    int i;

    for (i = 0; i < 256; i++)
        fbdvi_set_palette_rgb(i, 0, 0, 0);

    g_fbdvi_black_words[0] = g_fbdvi_palette_words[0][0];
    g_fbdvi_black_words[1] = g_fbdvi_palette_words[0][1];
    g_fbdvi_black_words[2] = g_fbdvi_palette_words[0][2];
}

static void fbdvi_fill_black_line(int buffer)
{
    int lane;
    int x;

    for (lane = 0; lane < FBDVI_LANE_COUNT; lane++) {
        for (x = 0; x < FBDVI_ACTIVE_WORDS; x++)
            g_fbdvi_line_buffer[buffer][lane][x] =
                g_fbdvi_black_words[lane];
    }
}

static int fbdvi_source_y_from_active_line(int active_line)
{
    active_line -= FBDVI_OUTPUT_TOP_BORDER;

    if ((active_line < 0) ||
        (active_line >= (FBDVI_FRAMEBUFFER_HEIGHT * FBDVI_OUTPUT_SCALE_Y)))
        return -1;

    return active_line / FBDVI_OUTPUT_SCALE_Y;
}

static void fbdvi_prepare_gfx_line(int buffer, int active_line)
{
    const unsigned char *src;
    int source_y;
    int x;

    if (!g_fbdvi_scanout_enabled || (__fb_gfx == NULL) ||
        (__fb_gfx->framebuffer == NULL) ||
        (__fb_gfx->w != FBDVI_FRAMEBUFFER_WIDTH) ||
        (__fb_gfx->h != FBDVI_FRAMEBUFFER_HEIGHT) ||
        (__fb_gfx->bpp != 1)) {
        fbdvi_fill_black_line(buffer);
        return;
    }

    source_y = fbdvi_source_y_from_active_line(active_line);

    if (source_y < 0) {
        fbdvi_fill_black_line(buffer);
        return;
    }

    src = __fb_gfx->framebuffer + ((size_t)source_y * (size_t)__fb_gfx->pitch);

    for (x = 0; x < FBDVI_ACTIVE_WORDS; x++) {
        unsigned int index;

        index = src[x];
        g_fbdvi_line_buffer[buffer][0][x] = g_fbdvi_palette_words[index][0];
        g_fbdvi_line_buffer[buffer][1][x] = g_fbdvi_palette_words[index][1];
        g_fbdvi_line_buffer[buffer][2][x] = g_fbdvi_palette_words[index][2];
    }
}

static void fbdvi_timing_state_advance(void)
{
    const struct fbdvi_timing *timing;
    uint16_t limit;

    timing = &g_fbdvi_timing_640x480;
    g_fbdvi_timing_state.v_ctr++;

    switch (g_fbdvi_timing_state.v_state) {
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
        return;

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
        ctrl |= read_ring << RP23XX_DMA_CTRL_TRIG_RING_SIZE_SHIFT;

    if (!irq_on_finish)
        ctrl |= RP23XX_DMA_CTRL_TRIG_IRQ_QUIET;

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
        hsync_off, timing->h_active_pixels / FBDVI_SYMBOLS_PER_WORD, 2,
        false);

    for (lane = 1; lane < FBDVI_LANE_COUNT; lane++) {
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

static void fbdvi_update_active_buffer(int buffer)
{
    struct fbdvi_dma_cb *lane_list;
    int lane;

    lane_list = fbdvi_lane_from_list(&g_fbdvi_active_scanline,
        FBDVI_SYNC_LANE);
    lane_list[3].read_addr =
        (uintptr_t)&g_fbdvi_line_buffer[buffer][FBDVI_SYNC_LANE][0];

    for (lane = 1; lane < FBDVI_LANE_COUNT; lane++) {
        lane_list = fbdvi_lane_from_list(&g_fbdvi_active_scanline, lane);
        lane_list[1].read_addr =
            (uintptr_t)&g_fbdvi_line_buffer[buffer][lane][0];
    }
}

static void fbdvi_setup_active_scanline(struct fbdvi_scanline_list *list,
    int buffer)
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
        &g_fbdvi_line_buffer[buffer][FBDVI_SYNC_LANE][0],
        FBDVI_ACTIVE_WORDS, 0, false);

    for (lane = 1; lane < FBDVI_LANE_COUNT; lane++) {
        lane_list = fbdvi_lane_from_list(list, lane);
        fbdvi_set_data_cb(&lane_list[0], &g_fbdvi_lanes[lane],
            no_sync,
            (timing->h_front_porch + timing->h_sync_width +
            timing->h_back_porch) / FBDVI_SYMBOLS_PER_WORD,
            2, false);
        fbdvi_set_data_cb(&lane_list[1], &g_fbdvi_lanes[lane],
            &g_fbdvi_line_buffer[buffer][lane][0],
            FBDVI_ACTIVE_WORDS, 0, false);
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
       The alias order is CTRL, READ_ADDR, WRITE_ADDR, TRANS_COUNT_TRIG. */

    putreg32(RP23XX_DMA_AL1_CTRL(lane->data_channel),
        RP23XX_DMA_WRITE_ADDR(lane->control_channel));
    putreg32(4, RP23XX_DMA_TRANS_COUNT(lane->control_channel));
    putreg32(fbdvi_control_ctrl(lane->control_channel),
        RP23XX_DMA_CTRL_TRIG(lane->control_channel));
}

static void fbdvi_enable_control_channel(struct fbdvi_lane *lane)
{
    /* CTRL_TRIG starts the channel as soon as the enable bit is written.
       The AL1_CTRL alias updates the same control word without triggering it,
       so the three lanes can still be started together below. */

    putreg32(fbdvi_control_ctrl(lane->control_channel) |
        RP23XX_DMA_CTRL_TRIG_EN,
        RP23XX_DMA_AL1_CTRL(lane->control_channel));
}

static void fbdvi_load_dma_list(struct fbdvi_scanline_list *list)
{
    struct fbdvi_lane *lane;
    int i;

    for (i = 0; i < FBDVI_LANE_COUNT; i++) {
        lane = &g_fbdvi_lanes[i];
        putreg32((uintptr_t)fbdvi_lane_from_list(list, i),
            RP23XX_DMA_READ_ADDR(lane->control_channel));
        putreg32(4, RP23XX_DMA_TRANS_COUNT(lane->control_channel));
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
       The scanline IRQ fires at the end of the sync lane's back porch block.
       Before repointing the control channels at the next scanline, PicoDVI
       waits until all three data channels have definitely loaded their active
       block.  Without this guard, the IRQ can race the non-sync lanes and make
       them skip or corrupt the active transfer, which quickly drains the PIO
       FIFOs. */

    for (i = 0; i < FBDVI_LANE_COUNT; i++) {
        guard = FBDVI_DMA_WAIT_GUARD;

        while (getreg32(RP23XX_DMA_DBG_TCR(g_fbdvi_lanes[i].data_channel)) !=
            expected_count) {
            guard--;

            if (guard == 0)
                break;
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
    putreg32(irq_bit, RP23XX_DMA_INTS1);

    fbdvi_timing_state_advance();
    fbdvi_wait_for_active_blocks_loaded();

    switch (g_fbdvi_timing_state.v_state) {
    case FBDVI_STATE_SYNC:
        next_list = &g_fbdvi_vblank_sync;
        break;

    case FBDVI_STATE_ACTIVE:
        {
            int current_buffer;
            int next_buffer;

            current_buffer = g_fbdvi_display_buffer;
            fbdvi_update_active_buffer(current_buffer);
            next_list = &g_fbdvi_active_scanline;

            next_buffer = current_buffer ^ 1;
            fbdvi_prepare_gfx_line(next_buffer,
                (int)g_fbdvi_timing_state.v_ctr + 1);
            g_fbdvi_display_buffer = next_buffer;
        }
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
       virtual pin numbers here, matching PicoDVI's pio_sm_set_* calls after
       it selects that GPIO base. */

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
        putreg32(g_fbdvi_serialiser_program[i],
            RP23XX_PIO_INSTR_MEM(FBDVI_PIO, i));

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

    while ((getreg32(RP23XX_PIO_FSTAT(FBDVI_PIO)) & full_mask) !=
        full_mask) {
        guard--;

        if (guard == 0)
            return -1;
    }

    return 0;
}

static void fbdvi_enable_pio(void)
{
    uint32_t sm_mask;

    sm_mask = (1u << 0) | (1u << 1) | (1u << 2);

    /*
       The three TMDS lanes must begin on the same PIO clock edge.  Match the
       vendor PicoDVI path by restarting the selected clock dividers and
       enabling all three state machines in one register write. */

    rp23xx_pio_enable_sm_mask_in_sync(FBDVI_PIO, sm_mask);
}

static int fbdvi_allocate_dma(void)
{
    int i;

    for (i = 0; i < FBDVI_LANE_COUNT; i++) {
        g_fbdvi_lanes[i].control = rp23xx_dmachannel();
        g_fbdvi_lanes[i].data = rp23xx_dmachannel();

        if (g_fbdvi_lanes[i].control == NULL ||
            g_fbdvi_lanes[i].data == NULL)
            return -1;

        g_fbdvi_lanes[i].control_channel =
            fbdvi_dma_channel(g_fbdvi_lanes[i].control);
        g_fbdvi_lanes[i].data_channel =
            fbdvi_dma_channel(g_fbdvi_lanes[i].data);
        g_fbdvi_lanes[i].tx_fifo = RP23XX_PIO_TXF(FBDVI_PIO, i);
        g_fbdvi_lanes[i].dreq = (uint8_t)(RP23XX_DMA_DREQ_PIO0_TX0 + i);

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
       PicoDVI gives its DMA IRQ a high priority so the scanline handler runs
       during the active-video window. NuttX initializes RP2350 external IRQs
       at one shared priority, which is too easy for USB console traffic to
       disturb during bring-up. */

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

    fbdvi_configure_control_channel(&g_fbdvi_lanes[0],
        &g_fbdvi_vblank_nosync);
    fbdvi_configure_control_channel(&g_fbdvi_lanes[1],
        &g_fbdvi_vblank_nosync);
    fbdvi_configure_control_channel(&g_fbdvi_lanes[2],
        &g_fbdvi_vblank_nosync);

    channel_mask = 0;

    for (i = 0; i < FBDVI_LANE_COUNT; i++) {
        fbdvi_enable_control_channel(&g_fbdvi_lanes[i]);
        channel_mask |= 1u << g_fbdvi_lanes[i].control_channel;
    }

    /* MULTI_CHAN_TRIGGER starts enabled channels.  The control channels were
       enabled through AL1_CTRL above so this write synchronizes the first load
       for all three TMDS lanes. */

    putreg32(channel_mask, RP23XX_DMA_MULTI_CHAN_TRIGGER);
}

int fb_nuttx_dvi_start(void)
{
    if (g_fbdvi_started)
        return 0;

    fbdvi_init_palette();
    fbdvi_fill_black_line(0);
    fbdvi_fill_black_line(1);
    g_fbdvi_display_buffer = 0;

    if (fbdvi_allocate_dma() < 0)
        return -1;

    fbdvi_configure_clock_gpio();
    fbdvi_configure_pwm_clock();
    fbdvi_configure_pio();

    fbdvi_setup_vblank(true, &g_fbdvi_vblank_sync);
    fbdvi_setup_vblank(false, &g_fbdvi_vblank_nosync);
    fbdvi_setup_active_scanline(&g_fbdvi_active_scanline, 0);

    g_fbdvi_timing_state.v_ctr = 0;
    g_fbdvi_timing_state.v_state = FBDVI_STATE_FRONT_PORCH;

    fbdvi_route_dma_irq();
    fbdvi_start_dma();

    if (fbdvi_wait_for_fifos_full() < 0)
        return -1;

    fbdvi_enable_pio();
    fbdvi_enable_pwm_clock();

    g_fbdvi_started = true;

    return 0;
}

void fb_nuttx_dvi_set_palette(int index, unsigned int rgb)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    red = (uint8_t)((rgb >> 16) & 0xffu);
    green = (uint8_t)((rgb >> 8) & 0xffu);
    blue = (uint8_t)(rgb & 0xffu);

    fbdvi_set_palette_rgb(index, red, green, blue);
}

void fb_nuttx_dvi_present(void)
{
    if (!g_fbdvi_started)
        return;

    g_fbdvi_scanout_enabled = 1;
}

void fb_nuttx_dvi_blank(void)
{
    g_fbdvi_scanout_enabled = 0;
}

#else

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES

#define FBDVI_FRAMEBUFFER_WIDTH 320
#define FBDVI_FRAMEBUFFER_HEIGHT 200
#define FBDVI_OUTPUT_TOP_BORDER 40
#define FBDVI_OUTPUT_SCALE_Y 2
#define FBDVI_ACTIVE_WORDS 320
#define FBDVI_LANE_COUNT 3
#define FBDVI_QEMU_SAMPLE_X 10
#define FBDVI_QEMU_SAMPLE_Y 10

static const uint32_t g_fbdvi_tmds_pair_table[64] =
{
    0x0007fd00, 0x0040dfc, 0x0041df8, 0x0007ed04,
    0x0043df0, 0x0007cd0c, 0x0007dd08, 0x0042df4,
    0x0047de0, 0x00078d1c, 0x00079d18, 0x0046de4,
    0x0007bd10, 0x0044dec, 0x0045de8, 0x000afa41,
    0x004fdc0, 0x00070d3c, 0x00071d38, 0x004edc4,
    0x00073d30, 0x004cdcc, 0x004ddc8, 0x000a7a61,
    0x00077d20, 0x0048ddc, 0x0049dd8, 0x000a3a71,
    0x004bdd0, 0x000a1a79, 0x000a0a7d, 0x0009fa81,
    0x005fd80, 0x00060d7c, 0x00061d78, 0x005ed84,
    0x00063d70, 0x005cd8c, 0x005dd88, 0x000b7a21,
    0x00067d60, 0x0058d9c, 0x0059d98, 0x000b3a31,
    0x005bd90, 0x000b1a39, 0x000b0a3d, 0x0008fac1,
    0x006fd40, 0x0050dbc, 0x0051db8, 0x000bba11,
    0x0053db0, 0x000b9a19, 0x000b8a1d, 0x00087ae1,
    0x0057da0, 0x000bda09, 0x000bca0d, 0x00083af1,
    0x000bea05, 0x00081af9, 0x00080afd, 0x000bfa01
};

static uint32_t g_fbdvi_qemu_palette_words[256][FBDVI_LANE_COUNT];
static unsigned int g_fbdvi_qemu_present_count;
static uint32_t g_fbdvi_qemu_last_checksum;
static int g_fbdvi_qemu_started;
static int g_fbdvi_qemu_scanout_enabled;

static uint32_t fbdvi_qemu_tmds_word(uint8_t value)
{
    return g_fbdvi_tmds_pair_table[value >> 2];
}

static void fbdvi_qemu_set_palette_rgb(int index, uint8_t red,
    uint8_t green, uint8_t blue)
{
    if ((index < 0) || (index >= 256))
        return;

    g_fbdvi_qemu_palette_words[index][0] = fbdvi_qemu_tmds_word(blue);
    g_fbdvi_qemu_palette_words[index][1] = fbdvi_qemu_tmds_word(green);
    g_fbdvi_qemu_palette_words[index][2] = fbdvi_qemu_tmds_word(red);
}

static void fbdvi_qemu_init_palette(void)
{
    int i;

    for (i = 0; i < 256; i++)
        fbdvi_qemu_set_palette_rgb(i, 0, 0, 0);
}

static uint32_t fbdvi_qemu_checksum_words(const unsigned char *src,
    unsigned int *nonblack)
{
    uint32_t hash;
    unsigned int count;
    int x;
    int lane;

    hash = 2166136261u;
    count = 0;

    for (x = 0; x < FBDVI_ACTIVE_WORDS; x++) {
        unsigned int index;

        index = src[x];

        if (index != 0)
            count++;

        for (lane = 0; lane < FBDVI_LANE_COUNT; lane++) {
            uint32_t word;

            word = g_fbdvi_qemu_palette_words[index][lane];
            hash ^= word;
            hash *= 16777619u;
        }
    }

    if (nonblack != NULL)
        *nonblack = count;

    return hash;
}

static void fbdvi_qemu_trace_scanout(void)
{
    const unsigned char *src;
    unsigned int index;
    unsigned int nonblack;
    uint32_t checksum;
    int source_y;
    int active_line;

    if (!g_fbdvi_qemu_started || !g_fbdvi_qemu_scanout_enabled)
        return;

    if ((__fb_gfx == NULL) || (__fb_gfx->framebuffer == NULL))
        return;

    if ((__fb_gfx->w != FBDVI_FRAMEBUFFER_WIDTH) ||
        (__fb_gfx->h != FBDVI_FRAMEBUFFER_HEIGHT) ||
        (__fb_gfx->bpp != 1) ||
        (__fb_gfx->pitch < FBDVI_FRAMEBUFFER_WIDTH))
        return;

    source_y = FBDVI_QEMU_SAMPLE_Y;
    active_line = FBDVI_OUTPUT_TOP_BORDER +
        (source_y * FBDVI_OUTPUT_SCALE_Y);
    src = __fb_gfx->framebuffer + ((size_t)source_y *
        (size_t)__fb_gfx->pitch);
    checksum = fbdvi_qemu_checksum_words(src, &nonblack);

    if ((g_fbdvi_qemu_present_count > 0) &&
        (checksum == g_fbdvi_qemu_last_checksum))
        return;

    g_fbdvi_qemu_present_count++;
    g_fbdvi_qemu_last_checksum = checksum;
    index = src[FBDVI_QEMU_SAMPLE_X];

    printf("FB_NUTTX_QEMU_DVI_SCANOUT frame=%u active_line=%d source_y=%d sample_x=%d index=%u blue=%08lx green=%08lx red=%08lx nonblack=%u checksum=%08lx\n",
        g_fbdvi_qemu_present_count,
        active_line,
        source_y,
        FBDVI_QEMU_SAMPLE_X,
        index,
        (unsigned long)g_fbdvi_qemu_palette_words[index][0],
        (unsigned long)g_fbdvi_qemu_palette_words[index][1],
        (unsigned long)g_fbdvi_qemu_palette_words[index][2],
        nonblack,
        (unsigned long)checksum);
    fflush(stdout);
}

int fb_nuttx_dvi_start(void)
{
    fbdvi_qemu_init_palette();
    g_fbdvi_qemu_present_count = 0;
    g_fbdvi_qemu_last_checksum = 0;
    g_fbdvi_qemu_scanout_enabled = 0;
    g_fbdvi_qemu_started = 1;

    return 0;
}

void fb_nuttx_dvi_set_palette(int index, unsigned int rgb)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    red = (uint8_t)((rgb >> 16) & 0xffu);
    green = (uint8_t)((rgb >> 8) & 0xffu);
    blue = (uint8_t)(rgb & 0xffu);

    fbdvi_qemu_set_palette_rgb(index, red, green, blue);
}

void fb_nuttx_dvi_present(void)
{
    g_fbdvi_qemu_scanout_enabled = 1;
    fbdvi_qemu_trace_scanout();
}

void fb_nuttx_dvi_blank(void)
{
    g_fbdvi_qemu_scanout_enabled = 0;
}

#else

int fb_nuttx_dvi_start(void)
{
    return -1;
}

void fb_nuttx_dvi_set_palette(int index, unsigned int rgb)
{
    (void)index;
    (void)rgb;
}

void fb_nuttx_dvi_present(void)
{
}

void fb_nuttx_dvi_blank(void)
{
}

#endif

#endif

/* end of gfx_rp2350_dvi.c */
