/* FreeBASIC Sound Library: dos/sfx_sb_irq.c
 *
 * Own the optional Sound Blaster completion interrupt. The interrupt only
 * acknowledges the DSP and sets a completion flag. The worker performs all
 * mixing, memory allocation, and waiting in ordinary thread context.
 */

#include "../fb_sfx_internal.h"

#if FB_SFX_DOS_THREADS
#include "../../rtlib/dos/fb_dos.h"
#include <dpmi.h>
#include <pc.h>

static volatile struct {
    int port;
    int done;
    unsigned int count;
} completion;
static int installed_irq = -1;
static unsigned char saved_mask;

/* GCC must keep the end marker after the ISR for DPMI code locking. */
static int __attribute__((no_reorder)) sb_interrupt(unsigned irq)
{
    (void)irq;
    (void)inportb(completion.port + 0x0E);
    completion.done = 1;
    completion.count++;
    return 1;
}

static void __attribute__((no_reorder)) sb_interrupt_end(void) { }

int fb_sfxMsdosIrqInit(int port, int irq)
{
    int interrupt_state;
    /* The classic DMA8 profile accepts ISA Sound Blaster IRQ5 or IRQ7.
     * Other BLASTER configurations retain synchronous playback.
     */
    if ((irq != 5 && irq != 7) || fb_isr_get(irq) != NULL)
        return -1;
    completion.port = port;
    completion.done = 1;
    completion.count = 0;
    if (fb_dos_lock_data((const void *)&completion, sizeof(completion)) != 0)
        return -1;
    if (!fb_isr_set(irq, sb_interrupt,
                   (const char *)sb_interrupt_end - (const char *)sb_interrupt, 4096))
    {
        fb_dos_unlock_data((const void *)&completion, sizeof(completion));
        return -1;
    }
    interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state();
    saved_mask = inportb(0x21) & (1 << irq);
    outportb(0x21, inportb(0x21) & ~(1 << irq));
    installed_irq = irq;
    __dpmi_get_and_set_virtual_interrupt_state(interrupt_state);
    return 0;
}

void fb_sfxMsdosIrqExit(void)
{
    int irq = installed_irq;
    int interrupt_state;
    if (irq < 0)
        return;
    interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state();
    outportb(0x21, (inportb(0x21) & ~(1 << irq)) | saved_mask);
    __dpmi_get_and_set_virtual_interrupt_state(interrupt_state);
    fb_isr_reset(irq);
    fb_dos_unlock_data((const void *)&completion, sizeof(completion));
    installed_irq = -1;
}

void fb_sfxMsdosIrqBegin(void) { completion.done = 0; }
int fb_sfxMsdosIrqDone(void) { return completion.done; }
unsigned int fb_sfxMsdosIrqCount(void) { return completion.count; }

#endif

/* end of sfx_sb_irq.c */
