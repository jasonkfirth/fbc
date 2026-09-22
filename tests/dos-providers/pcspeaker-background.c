/* FreeBASIC DOS provider tests: pcspeaker-background.c
 *
 * Exercise real speaker output with simultaneous foreground work, locked
 * runtime sections, queue starvation/recovery, and repeated initialization.
 * The harness disables Sound Blaster. Emulator WAV capture verifies the
 * hardware output separately from the mixer's accepted-PCM capture.
 */

#include "../../src/rtlib/fb.h"
#include "../../src/rtlib/fb_private_thread.h"
#include "../../src/sfxlib/fb_sfx.h"
#include "../../src/sfxlib/fb_sfx_internal.h"
#include "../../src/sfxlib/dos/fb_sfx_msdos.h"
#include <pc.h>
#include <time.h>

static unsigned char rtc_read(unsigned char reg)
{
    unsigned char value;
    int state = __dpmi_get_and_disable_virtual_interrupt_state();
    outportb(0x70, reg);
    value = inportb(0x71);
    __dpmi_get_and_set_virtual_interrupt_state(state);
    return value;
}

static void rtc_write(unsigned char reg, unsigned char value)
{
    int state = __dpmi_get_and_disable_virtual_interrupt_state();
    outportb(0x70, reg);
    outportb(0x71, value);
    __dpmi_get_and_set_virtual_interrupt_state(state);
}

static unsigned long busy(unsigned int msecs)
{
    uclock_t start = uclock();
    uclock_t duration = (uclock_t)msecs * UCLOCKS_PER_SEC / 1000;
    volatile unsigned long count = 0;
    while (uclock() - start < duration)
    {
        count++;
    }
    return count;
}

static void chord(void)
{
    fb_sfxSoundChannel(0, 440, 3.0f, 0.5f);
    fb_sfxSoundChannel(1, 660, 3.0f, 0.3f);
    fb_sfxSoundChannel(2, 880, 3.0f, 0.2f);
}

static int require(int condition, const char *description)
{
    if (!condition)
        printf("FAIL %s\n", description);
    return condition;
}

static void phase(int cycle, const char *description)
{
    FILE *file;
    /* Close each entry so a watchdog can recover progress from a FAT image. */
    fb_Lock();
    file = fopen("PHASE.TXT", "a");
    if (file)
    {
        fprintf(file, "cycle %d: %s\n", cycle + 1, description);
        fclose(file);
    }
    fb_Unlock();
}

int main(int argc, char **argv)
{
    unsigned char speaker_bits, saved_b;
    unsigned long iterations = 0;
    int cycle;
    setvbuf(stdout, NULL, _IONBF, 0);
    fb_hRtInit();
    speaker_bits = inportb(0x61) & 3;
    saved_b = rtc_read(0x0B);

    if (argc > 1)
    {
        int slow = strcmp(argv[1], "slow") == 0;
        if (slow && !fb_DosThreadInit())
            return 1;
        saved_b = rtc_read(0x0B);
        /* An enabled SQW pin prevents rate sharing, while an existing PIE
         * owner prevents scheduler initialization altogether.
         */
        rtc_write(0x0B, saved_b | (slow ? 0x08 : 0x40));
        if (!require(fb_sfxInit() == 0 && __fb_sfx &&
                     strcmp(__fb_sfx->driver->name, "PCSpeaker") == 0 &&
                     !fb_sfxMsdosPcSpeakerIrqActive() &&
                     !!fb_sfxMsdosWorkerActive() == slow, "speaker fallback selection"))
            return 1;
        chord();
        if (slow)
            iterations = busy(1000);
        else
            fb_Delay(250);
        fb_sfxSoundLegacy2(880, 2);
        fb_sfxExit();
        if (!require(rtc_read(0x0B) == (saved_b | (slow ? 0x08 : 0x40)),
                     "fallback preserves RTC ownership"))
            return 1;
        rtc_write(0x0B, saved_b);
        printf("PASS speaker fallback, foreground iterations=%lu\n", iterations);
        fb_hRtExit();
        return 0;
    }

    for (cycle = 0; cycle < 3; cycle++)
    {
        unsigned int consumed, locked_samples, underruns;
        double wall_started, wall_elapsed;
        uclock_t pit_started, pit_elapsed;
        float block[1024];
        int i;
        phase(cycle, "initializing");
        if (!require(fb_sfxInit() == 0 && __fb_sfx &&
                     strcmp(__fb_sfx->driver->name, "PCSpeaker") == 0 &&
                     fb_sfxMsdosWorkerActive() && fb_sfxMsdosPcSpeakerIrqActive() &&
                     __fb_sfx->samplerate == 8192, "clocked speaker worker initialization"))
            return 1;
        if (!require(fb_sfxOutputCaptureStart() == 0, "capture start"))
            return 1;
        chord();
        phase(cycle, "background spin");
        wall_started = fb_Timer();
        pit_started = uclock();
        consumed = fb_sfxMsdosPcSpeakerSamples();
        iterations += busy(750);
        consumed = fb_sfxMsdosPcSpeakerSamples() - consumed;
        pit_elapsed = uclock() - pit_started;
        wall_elapsed = fb_Timer() - wall_started;
        phase(cycle, "spin complete");
        /* Allow scheduling startup and emulation jitter, but require more
         * than a few incidental samples during the CPU-bound interval.
         */
        if (!require(consumed > 4000 && consumed < 7000 &&
                     pit_elapsed < UCLOCKS_PER_SEC && wall_elapsed < 1.0,
                     "background sample clock"))
        {
            printf("samples=%u underruns=%u RTC=%02x/%02x PIT=%.6f TIMER=%.6f\n",
                consumed, fb_sfxMsdosPcSpeakerUnderruns(), rtc_read(0x0A), rtc_read(0x0B),
                (double)pit_elapsed / UCLOCKS_PER_SEC, wall_elapsed);
            return 1;
        }
        fb_sfxOutputCaptureStop();
        phase(cycle, "saving capture");
        if (!require(fb_sfxOutputCaptureSave(cycle ? "REPEAT.WAV" : "AUDIO.WAV") == 0,
                     "capture save"))
            return 1;

        /* Stop the producer and drain its tail. Then exclude scheduling and
         * consume a known buffer: progress here must come from the ISR.
         */
        fb_sfxPlatformExit();
        phase(cycle, "worker stopped");
        for (i = 0; i < 1024; i++)
            block[i] = (i / 8) & 1 ? 0.5f : -0.5f;
        if (fb_sfxMsdosPcSpeakerIrqWrite(block, 1, 1, 1) != 1)
            return 1;
        fb_Lock();
        consumed = fb_sfxMsdosPcSpeakerSamples();
        if (fb_sfxMsdosPcSpeakerIrqWrite(block, 1024, 1, 0) != 1024)
            return 1;
        busy(50);
        locked_samples = fb_sfxMsdosPcSpeakerSamples() - consumed;
        fb_Unlock();
        phase(cycle, "locked samples consumed");
        if (!require(locked_samples > 300 && locked_samples < 550,
                     "samples advance with task switching excluded"))
            return 1;
        underruns = fb_sfxMsdosPcSpeakerUnderruns();
        fb_DosThreadDelay(250);
        if (!require(fb_sfxMsdosPcSpeakerSamples() - consumed == 1024 &&
                     fb_sfxMsdosPcSpeakerUnderruns() > underruns &&
                     !(inportb(0x61) & 3), "empty queue drives silence"))
            return 1;
        consumed = fb_sfxMsdosPcSpeakerSamples();
        if (fb_sfxMsdosStartWorker() != 0)
            return 1;
        chord();
        phase(cycle, "recovered worker");
        busy(300);
        if (!require(fb_sfxMsdosPcSpeakerSamples() - consumed > 1500,
                     "starved queue recovers"))
            return 1;
        fb_sfxSoundLegacy2(880, 2);
        phase(cycle, "foreground sound complete");
        fb_sfxExit();
        phase(cycle, "driver stopped");
        if (!require(!fb_sfxMsdosPcSpeakerIrqActive() &&
                     (rtc_read(0x0A) & 15) == RTC128 &&
                     (inportb(0x61) & 3) == speaker_bits,
                     "speaker shutdown restores state and scheduler rate"))
            return 1;
        printf("PASS speaker cycle %d, locked samples=%u\n", cycle + 1, locked_samples);
    }
    if (!require(iterations > 0, "foreground computation"))
        return 1;
    printf("PASS clocked background PC speaker, foreground iterations=%lu\n", iterations);
    fb_hRtExit();
    return 0;
}

/* end of pcspeaker-background.c */
