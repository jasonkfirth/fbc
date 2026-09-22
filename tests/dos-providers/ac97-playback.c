/* FreeBASIC DOS audio tests: ac97-playback.c
 *
 * Exercise real AC'97 DMA, discovery rejection, driver priority and the DOS
 * sound worker. Hardware output is captured by the emulator, independently
 * of the mixer. This file contains no replacement device implementation.
 */

#include "../../src/rtlib/fb.h"
#include "../../src/sfxlib/fb_sfx.h"
#include "../../src/sfxlib/fb_sfx_internal.h"
#include "../../src/sfxlib/fb_sfx_driver.h"
#include <math.h>
#include <limits.h>
#include <dpmi.h>
#include <pc.h>
#include <time.h>

extern const FB_SFX_DRIVER fb_sfxDriverAc97;

static int pci_config(unsigned short device, unsigned short service,
                       unsigned short offset, unsigned long *value)
{
    __dpmi_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = service;
    regs.x.bx = device;
    regs.x.di = offset;
    regs.d.ecx = *value;
    if (__dpmi_int(0x1A, &regs) != 0 || (regs.x.flags & 1) || regs.h.ah)
        return 0;
    *value = regs.d.ecx;
    return 1;
}

static int resource_tests(void)
{
    __dpmi_regs regs;
    unsigned short device;
    unsigned long mixer = 0, busmaster = 0, command = 0, value;
    float pcm[2048] = { 0 };
    uclock_t start;
    int rejected, result;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0xB102; /* The QEMU test controller is Intel 82801AA. */
    regs.x.cx = 0x2415;
    regs.x.dx = 0x8086;
    if (__dpmi_int(0x1A, &regs) != 0 || (regs.x.flags & 1) || regs.h.ah)
        return 1;
    device = regs.x.bx;
    if (!pci_config(device, 0xB10A, 0x10, &mixer) ||
        !pci_config(device, 0xB10A, 0x14, &busmaster) ||
        !pci_config(device, 0xB10A, 4, &command))
        return 1;
    command &= 0xFFFF;

    /* These destructive resource probes run only in the disposable guest.
     * Disable I/O decoding before moving BARs, restore each BAR afterwards,
     * and require rejection rather than accesses through invalid resources.
     */
    value = command & ~1UL;
    if (!pci_config(device, 0xB10C, 4, &value))
        return 1;
    value = 1; /* Unassigned NAMBAR. */
    if (!pci_config(device, 0xB10D, 0x10, &value))
        return 1;
    rejected = fb_sfxDriverAc97.init(48000, 2, 1024, 0) != 0;
    fb_sfxDriverAc97.exit();
    value = mixer;
    if (!pci_config(device, 0xB10D, 0x10, &value) || !rejected)
        return 1;
    value = mixer; /* Force NABMBAR to overlap the mixer window. */
    if (!pci_config(device, 0xB10D, 0x14, &value))
        return 1;
    rejected = fb_sfxDriverAc97.init(48000, 2, 1024, 0) != 0;
    fb_sfxDriverAc97.exit();
    value = busmaster;
    if (!pci_config(device, 0xB10D, 0x14, &value) || !rejected)
        return 1;
    value = command;
    if (!pci_config(device, 0xB10C, 4, &value))
        return 1;
    puts("PASS unassigned and overlapping PCI resources rejected");

    if (fb_sfxDriverAc97.init(48000, 2, 1024, 0) != 0)
        return 1;
    value = (command | 1) & ~4UL; /* Simulate a lost PCI DMA grant. */
    if (!pci_config(device, 0xB10C, 4, &value))
        return 1;
    start = uclock();
    result = fb_sfxDriverAc97.write(pcm, 1024);
    if (result >= 0 || uclock() - start > UCLOCKS_PER_SEC * 2)
        return 1;
    fb_sfxDriverAc97.exit();
    value = 0;
    if (!pci_config(device, 0xB10A, 4, &value) || (value & 0xFFFF) != command)
        return 1;
    if (fb_sfxDriverAc97.init(48000, 2, 1024, 0) != 0 ||
        fb_sfxDriverAc97.write(pcm, 1024) != 1024)
        return 1;
    fb_sfxDriverAc97.exit();
    puts("PASS stalled DMA times out, releases device and reinitializes");
    return 0;
}

static int direct_playback(void)
{
    float pcm[3000 * 2];
    int cycle, frame, i, count;
    /* Exceed the staging buffer and wrap the 32-entry BDL repeatedly.
     * Alternating mono/stereo also checks that channel indexing is local
     * to driver initialization, not dependent on an allocated sound context.
     */
    for (cycle = 0; cycle < 3; ++cycle)
    {
        int channels = cycle == 1 ? 1 : 2;
        if (fb_sfxDriverAc97.init(44100, channels, 1024, 0) != 0)
            return 1;
        if (fb_sfxDriverAc97.init(48000, channels, 1024, 0) == 0 ||
            fb_sfxDriverAc97.write(NULL, 16) >= 0 ||
            fb_sfxDriverAc97.write(pcm, 0) >= 0 ||
            fb_sfxDriverAc97.write(pcm, INT_MAX) >= 0)
            return 1;
        for (frame = 0; frame < 48000; frame += count)
        {
            count = 3000;
            if (count > 48000 - frame)
                count = 48000 - frame;
            for (i = 0; i < count; ++i)
            {
                pcm[i * channels] = 0.5f * sin(2.0 * 3.141592653589793 * 440 * (frame + i) / 48000);
                if (channels == 2)
                    pcm[i * 2 + 1] = 0.5f * sin(2.0 * 3.141592653589793 * 880 * (frame + i) / 48000);
            }
            if (fb_sfxDriverAc97.write(pcm, count) != count)
                return 1;
        }
        fb_sfxDriverAc97.exit();
        fb_sfxDriverAc97.exit(); /* Shutdown is safe after partial/closed init. */
        if (fb_sfxDriverAc97.write(pcm, 16) >= 0)
            return 1;
        printf("PASS AC97 DMA cycle %d (%s)\n", cycle + 1, channels == 1 ? "mono" : "stereo");
    }
    return 0;
}

static int runtime_playback(int worker, const char *expected)
{
    int cycle;
    for (cycle = 0; cycle < 3; ++cycle)
    {
        uclock_t start;
        volatile unsigned long iterations = 0;
        if (fb_sfxInit() != 0 || !__fb_sfx || !__fb_sfx->driver ||
            strcmp(__fb_sfx->driver->name, expected) != 0)
            return 1;
        if (strcmp(expected, "AC97") == 0 && __fb_sfx->samplerate != 48000)
            return 1;
        printf("Selected %s, rate %d\n", __fb_sfx->driver->name, __fb_sfx->samplerate);
#if FB_SFX_DOS_THREADS
        if (worker && !fb_sfxMsdosWorkerActive())
            return 1;
#else
        if (worker)
            return 1;
#endif
        if (fb_sfxOutputCaptureStart() != 0)
            return 1;
        fb_sfxSoundChannel(0, 440, 1.0f, 0.5f);
        if (worker)
        {
            start = uclock();
            while (uclock() - start < UCLOCKS_PER_SEC * 3 / 4)
                iterations++;
            if (!iterations)
                return 1;
        }
        else
            fb_Delay(750);
        fb_sfxOutputCaptureStop();
        if (fb_sfxOutputCaptureSave(cycle == 0 ? "AUDIO.WAV" : "REPEAT.WAV") != 0 ||
            strcmp(__fb_sfx->driver->name, expected) != 0)
            return 1;
        fb_sfxSoundLegacy2(880, 2);
        fb_sfxExit();
        printf("PASS %s %s cycle %d\n", expected, worker ? "worker" : "foreground", cycle + 1);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int result = 1;
    const char *mode = argc > 1 ? argv[1] : "direct";
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!freopen("ERROR.TXT", "w", stderr))
        return 1;
    fb_hRtInit();
    if (strcmp(mode, "absent") == 0)
    {
        int i;
        result = 0;
        for (i = 0; i < 3; ++i)
        {
            if (fb_sfxDriverAc97.init(48000, 2, 1024, 0) == 0 ||
                fb_sfxDriverAc97.write(NULL, 16) >= 0)
                result = 1;
            fb_sfxDriverAc97.exit();
        }
        /* The environment request must not bypass actual device discovery. */
        setenv("SFXLIB_DRIVER", "AC97", 1);
        if (fb_sfxInit() != 0 || strcmp(__fb_sfx->driver->name, "PCSpeaker") != 0)
            result = 1;
        fb_sfxExit();
        if (!result)
            puts("PASS absent AC97 rejected, forced request falls back to PCSpeaker");
    }
    else if (strcmp(mode, "direct") == 0)
        result = direct_playback();
    else if (strcmp(mode, "resources") == 0)
        result = resource_tests();
    else if (strcmp(mode, "foreground") == 0 || strcmp(mode, "worker") == 0)
        result = runtime_playback(strcmp(mode, "worker") == 0, "AC97");
    else if (strcmp(mode, "sb") == 0)
        result = runtime_playback(0, "SoundBlaster");
    if (result)
        puts("FAIL AC97 probe");
    else
        puts("PASS AC97 probe");
    fb_hRtExit();
    return result;
}

/* end of ac97-playback.c */
