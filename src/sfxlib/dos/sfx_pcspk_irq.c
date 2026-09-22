/* FreeBASIC Sound Library: dos/sfx_pcspk_irq.c
 *
 * Clock queued one-bit PC speaker samples from the optional provider's RTC
 * interrupt. The audio worker mixes in ordinary thread context; this file
 * owns only the locked queue, integer output ISR, and bounded queue writes.
 * It neither changes PIT channels nor calls the mixer from an interrupt.
 */

#include "../fb_sfx_internal.h"
#include "fb_sfx_msdos.h"

#if FB_SFX_DOS_THREADS
#include "../../rtlib/dos/fb_dos.h"
#include "../../rtlib/dos/fb_dos_thread.h"
#include <pc.h>
#include <time.h>

/* 125 ms at 8192 Hz covers several 128 Hz scheduler slices. The power-of-two
 * capacity lets unsigned sequence counters wrap without changing slot order.
 * DriverIoLock serializes producers. Only the IRQ advances read_position.
 */
#define PCSPK_QUEUE_SIZE 1024U
#define PCSPK_PORT 0x61

static struct {
    volatile unsigned int read_position;
    volatile unsigned int write_position;
    volatile unsigned int underruns;
    unsigned char samples[PCSPK_QUEUE_SIZE];
} speaker_queue;
static int installed;

/* All referenced data and this exact code range are DPMI-locked. Keep the
 * handler leaf and integer-only: the interrupted thread owns the x87 state.
 * An empty queue drives silence instead of repeating a stale sample.
 */
static void __attribute__((no_reorder)) pcspk_interrupt(void)
{
    unsigned int position = speaker_queue.read_position;
    unsigned char level = 0;
    if (position != speaker_queue.write_position)
    {
        level = speaker_queue.samples[position & (PCSPK_QUEUE_SIZE - 1)];
        speaker_queue.read_position = position + 1;
    }
    else
        speaker_queue.underruns++;
    outportb(PCSPK_PORT, (inportb(PCSPK_PORT) & ~3U) | level);
}

static void __attribute__((no_reorder)) pcspk_interrupt_end(void) { }

int fb_sfxMsdosPcSpeakerIrqInit(void)
{
    size_t code_size = (const char *)pcspk_interrupt_end - (const char *)pcspk_interrupt;
    if (installed || !fb_DosThreadInit())
        return -1;
    memset(&speaker_queue, 0, sizeof(speaker_queue));
    if (fb_dos_lock_data(&speaker_queue, sizeof(speaker_queue)) != 0)
        return -1;
    if (fb_dos_lock_code(pcspk_interrupt, code_size) != 0)
        goto fail_data;
    if (!lwp_set_tick_hook(pcspk_interrupt, RTC8192))
        goto fail_code;
    installed = 1;
    return 0;

fail_code:
    fb_dos_unlock_code(pcspk_interrupt, code_size);
fail_data:
    fb_dos_unlock_data(&speaker_queue, sizeof(speaker_queue));
    return -1;
}

void fb_sfxMsdosPcSpeakerIrqExit(void)
{
    if (!installed)
        return;
    /* Remove the consumer before unlocking its code or queue. The caller
     * joins the worker and owns DriverIoLock before driver shutdown.
     */
    lwp_clear_tick_hook(pcspk_interrupt);
    installed = 0;
    fb_dos_unlock_code(pcspk_interrupt,
        (const char *)pcspk_interrupt_end - (const char *)pcspk_interrupt);
    fb_dos_unlock_data(&speaker_queue, sizeof(speaker_queue));
}

int fb_sfxMsdosPcSpeakerIrqActive(void) { return installed; }
unsigned int fb_sfxMsdosPcSpeakerSamples(void) { return speaker_queue.read_position; }
unsigned int fb_sfxMsdosPcSpeakerUnderruns(void) { return speaker_queue.underruns; }

/* Wait with a PIT-based watchdog. BIOS delay() uses this same RTC, and the
 * public fb_Delay() can re-enter the mixer, so neither is suitable here.
 */
static int pcspk_wait(unsigned int *last_read, uclock_t *last_progress)
{
    unsigned int position = speaker_queue.read_position;
    uclock_t now = uclock();
    if (position != *last_read)
    {
        *last_read = position;
        *last_progress = now;
    }
    else if (now - *last_progress > UCLOCKS_PER_SEC)
        return -1;
    fb_DosThreadDelay(1);
    return 0;
}

int fb_sfxMsdosPcSpeakerIrqWrite(const float *samples, int frames, int channels, int drain)
{
    int written = 0;
    unsigned int last_read = speaker_queue.read_position;
    uclock_t last_progress = uclock();
    if (!installed || !samples || frames <= 0 || channels <= 0 ||
        channels > INT_MAX / frames)
        return -1;
    while (written < frames)
    {
        unsigned int position = speaker_queue.write_position;
        unsigned int used = position - speaker_queue.read_position;
        unsigned int available;
        if (used > PCSPK_QUEUE_SIZE)
            return -1;
        available = PCSPK_QUEUE_SIZE - used;
        if (!available)
        {
            if (pcspk_wait(&last_read, &last_progress) != 0)
                return -1;
            continue;
        }
        while (available && written < frames)
        {
            float mixed = 0.0f;
            int channel;
            for (channel = 0; channel < channels; channel++)
                mixed += samples[written * channels + channel];
            speaker_queue.samples[position & (PCSPK_QUEUE_SIZE - 1)] =
                mixed > 0.0f ? 2 : 0;
            position++;
            written++;
            available--;
        }
        /* Publish only complete slots. The IRQ may read while the producer
         * fills unused slots, but cannot see those slots before this store.
         */
        __asm__ __volatile__("" : : : "memory");
        speaker_queue.write_position = position;
    }
    /* Foreground SOUND/PLAY must consume their tail before returning. The
     * background worker instead refills ahead, bounded by the queue capacity.
     */
    while (drain && speaker_queue.read_position != speaker_queue.write_position)
    {
        if (pcspk_wait(&last_read, &last_progress) != 0)
            return -1;
    }
    return written;
}

#endif

/* end of sfx_pcspk_irq.c */
