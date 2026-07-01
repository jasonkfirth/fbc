/*
    FreeBASIC NuttX/RP2350-PiZero DVI register dump
    ------------------------------------------------

    File: fbdvi_regs.c

    Purpose:

        Dump the small set of RP2350 registers involved in the temporary
        RP2350-PiZero DVI bring-up path.

    Responsibilities:

        - report GPIO mux and pad state for the DVI connector pins
        - report PWM clock-slice state for the differential pixel clock
        - report PIO0 state-machine configuration and FIFO state
        - report active DMA channel control state

    This file intentionally does NOT contain:

        - DVI signal generation
        - register modification helpers
        - a general RP23xx register inspection shell
*/

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <arch/chip/irq.h>
#include <nuttx/arch.h>

#include "hardware/rp23xx_dma.h"
#include "hardware/rp23xx_io_bank0.h"
#include "hardware/rp23xx_pads_bank0.h"
#include "hardware/rp23xx_pio.h"
#include "hardware/rp23xx_pwm.h"

#ifndef FAR
#  define FAR
#endif

#define FBDVI_PIO 0

#define FBDVI_GPIO_RED 32
#define FBDVI_GPIO_GREEN 34
#define FBDVI_GPIO_BLUE 36
#define FBDVI_GPIO_CLOCK 38

#define FBDVI_PWM_FIRST_SLICE 8
#define FBDVI_PWM_LAST_SLICE 11
#define FBDVI_DMA_CHANNELS 16
#define FBDVI_HAZARD3_MEIPRA_CSR "0xbe3"

static uint32_t fbdviregs_hazard3_irqarray_read_meipra(uint32_t index)
{
  uint32_t value;

  /* MEIPRA is Hazard3's external interrupt priority array. */

  __asm__ __volatile__("csrrs %0, " FBDVI_HAZARD3_MEIPRA_CSR ", %1" :
      "=r"(value) : "rK"(index) : "memory");

  return value >> 16;
}

static uint32_t fbdviregs_irq_priority(int irq)
{
  uint32_t extirq;
  uint32_t priority_word;
  uint32_t priority_shift;

  extirq = (uint32_t)(irq - RP23XX_IRQ_EXTINT);
  priority_word = fbdviregs_hazard3_irqarray_read_meipra(extirq / 4u);
  priority_shift = 4u * (extirq % 4u);

  return (priority_word >> priority_shift) & 0x0fu;
}

static void fbdviregs_dump_gpio(unsigned int gpio)
{
  printf("gpio%02u status=%08" PRIx32 " ctrl=%08" PRIx32
      " pad=%08" PRIx32 "\n",
      gpio,
      getreg32(RP23XX_IO_BANK0_GPIO_STATUS(gpio)),
      getreg32(RP23XX_IO_BANK0_GPIO_CTRL(gpio)),
      getreg32(RP23XX_PADS_BANK0_GPIO(gpio)));
}

static void fbdviregs_dump_pwm(unsigned int slice)
{
  printf("pwm%02u csr=%08" PRIx32 " div=%08" PRIx32
      " ctr=%08" PRIx32 " cc=%08" PRIx32 " top=%08" PRIx32 "\n",
      slice,
      getreg32(RP23XX_RV_PWM_CSR(slice)),
      getreg32(RP23XX_RV_PWM_DIV(slice)),
      getreg32(RP23XX_RV_PWM_CTR(slice)),
      getreg32(RP23XX_RV_PWM_CC(slice)),
      getreg32(RP23XX_RV_PWM_TOP(slice)));
}

static void fbdviregs_dump_pio_sm(unsigned int sm)
{
  printf("pio0.sm%u clkdiv=%08" PRIx32 " exec=%08" PRIx32
      " shift=%08" PRIx32 " pinctrl=%08" PRIx32
      " addr=%08" PRIx32 "\n",
      sm,
      getreg32(RP23XX_PIO_SM_CLKDIV(FBDVI_PIO, sm)),
      getreg32(RP23XX_PIO_SM_EXECCTRL(FBDVI_PIO, sm)),
      getreg32(RP23XX_PIO_SM_SHIFTCTRL(FBDVI_PIO, sm)),
      getreg32(RP23XX_PIO_SM_PINCTRL(FBDVI_PIO, sm)),
      getreg32(RP23XX_PIO_SM_ADDR(FBDVI_PIO, sm)));
}

static void fbdviregs_dump_dma(unsigned int channel, bool force)
{
  uint32_t read_addr;
  uint32_t write_addr;
  uint32_t count;
  uint32_t ctrl;

  read_addr = getreg32(RP23XX_DMA_READ_ADDR(channel));
  write_addr = getreg32(RP23XX_DMA_WRITE_ADDR(channel));
  count = getreg32(RP23XX_DMA_TRANS_COUNT(channel));
  ctrl = getreg32(RP23XX_DMA_CTRL_TRIG(channel));

  if (!force && (read_addr == 0) && (write_addr == 0) && (count == 0) &&
      (ctrl == 0))
    {
      return;
    }

  printf("dma%02u rd=%08" PRIx32 " wr=%08" PRIx32
      " cnt=%08" PRIx32 " ctrl=%08" PRIx32
      " tcr=%08" PRIx32 " dreq=%08" PRIx32 "\n",
      channel, read_addr, write_addr, count, ctrl,
      getreg32(RP23XX_DMA_DBG_TCR(channel)),
      getreg32(RP23XX_DMA_DBG_CTDREQ(channel)));
}

int main(int argc, FAR char *argv[])
{
  unsigned int i;

  (void)argc;
  (void)argv;

  printf("fbdviregs: RP2350-PiZero DVI register snapshot\n");
  printf("irq dma1 priority=%08" PRIx32 "\n",
      fbdviregs_irq_priority(RP23XX_DMA_IRQ_1));

  fbdviregs_dump_gpio(FBDVI_GPIO_RED);
  fbdviregs_dump_gpio(FBDVI_GPIO_RED + 1);
  fbdviregs_dump_gpio(FBDVI_GPIO_GREEN);
  fbdviregs_dump_gpio(FBDVI_GPIO_GREEN + 1);
  fbdviregs_dump_gpio(FBDVI_GPIO_BLUE);
  fbdviregs_dump_gpio(FBDVI_GPIO_BLUE + 1);
  fbdviregs_dump_gpio(FBDVI_GPIO_CLOCK);
  fbdviregs_dump_gpio(FBDVI_GPIO_CLOCK + 1);

  printf("pwm.en=%08" PRIx32 " intr=%08" PRIx32 "\n",
      getreg32(RP23XX_RV_PWM_EN), getreg32(RP23XX_RV_PWM_INTR));

  for (i = FBDVI_PWM_FIRST_SLICE; i <= FBDVI_PWM_LAST_SLICE; i++)
    {
      fbdviregs_dump_pwm(i);
    }

  printf("pio0 ctrl=%08" PRIx32 " fstat=%08" PRIx32
      " fdebug=%08" PRIx32 " flevel=%08" PRIx32
      " padout=%08" PRIx32 " padoe=%08" PRIx32
      " gpiobase=%08" PRIx32 "\n",
      getreg32(RP23XX_PIO_CTRL(FBDVI_PIO)),
      getreg32(RP23XX_PIO_FSTAT(FBDVI_PIO)),
      getreg32(RP23XX_PIO_FDEBUG(FBDVI_PIO)),
      getreg32(RP23XX_PIO_FLEVEL(FBDVI_PIO)),
      getreg32(RP23XX_PIO_DBG_PADOUT(FBDVI_PIO)),
      getreg32(RP23XX_PIO_DBG_PADOE(FBDVI_PIO)),
      getreg32(RP23XX_PIO_GPIOBASE(FBDVI_PIO)));

  for (i = 0; i < 3; i++)
    {
      fbdviregs_dump_pio_sm(i);
    }

  printf("dma intr=%08" PRIx32 " inte0=%08" PRIx32
      " inte1=%08" PRIx32 " ints0=%08" PRIx32
      " ints1=%08" PRIx32 " fifo=%08" PRIx32 "\n",
      getreg32(RP23XX_DMA_INTR),
      getreg32(RP23XX_DMA_INTE0),
      getreg32(RP23XX_DMA_INTE1),
      getreg32(RP23XX_DMA_INTS0),
      getreg32(RP23XX_DMA_INTS1),
      getreg32(RP23XX_DMA_FIFO_LEVELS));

  for (i = 0; i < FBDVI_DMA_CHANNELS; i++)
    {
      fbdviregs_dump_dma(i, i < 8);
    }

  return 0;
}

/* end of fbdvi_regs.c */
