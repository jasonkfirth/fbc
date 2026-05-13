/*
    DOS PC speaker fallback driver.

    This backend uses the direct PC speaker data bit as a 1-bit
    output device. It is intentionally simple and heavily
    bandwidth-limited, but it gives DOS builds a real fallback
    backend when BLASTER is not available.
*/

#ifndef DISABLE_MSDOS

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"

#include <time.h>

#ifdef __DJGPP__
#include <dos.h>
#endif

#define FB_SFX_MSDOS_SPEAKER_PORT   0x61

static int g_fb_sfx_pcspk_active = 0;
static int g_fb_sfx_pcspk_rate = 0;
static double g_fb_sfx_pcspk_ticks_per_sample = 0.0;
static double g_fb_sfx_pcspk_next_tick = 0.0;
static unsigned char g_fb_sfx_pcspk_saved_port61 = 0;
static unsigned char g_fb_sfx_pcspk_base_port61 = 0;

#ifdef __DJGPP__

static void fb_sfxPcSpeakerWaitForNextSample(void)
{
    uclock_t now;

    if (g_fb_sfx_pcspk_ticks_per_sample <= 0.0)
        return;

    if (g_fb_sfx_pcspk_next_tick <= 0.0)
    {
        g_fb_sfx_pcspk_next_tick = (double)uclock();
        return;
    }

    do
    {
        now = uclock();
    } while ((double)now < g_fb_sfx_pcspk_next_tick);

    g_fb_sfx_pcspk_next_tick += g_fb_sfx_pcspk_ticks_per_sample;
    if (g_fb_sfx_pcspk_next_tick < (double)now)
        g_fb_sfx_pcspk_next_tick = (double)now + g_fb_sfx_pcspk_ticks_per_sample;
}

static void fb_sfxPcSpeakerWriteLevel(int high)
{
    unsigned char port_value;

    /*
        Direct speaker output avoids the audible high-frequency PWM chatter
        produced by repeatedly retriggering PIT channel 2 in one-shot mode.
        Bit 0 gates PIT channel 2; clearing it leaves bit 1 as the direct
        speaker data line.
    */

    port_value = (unsigned char)(g_fb_sfx_pcspk_base_port61 & ~0x03u);
    if (high)
        port_value |= 0x02u;

    outportb(FB_SFX_MSDOS_SPEAKER_PORT, port_value);
}

static int msdos_pcspk_init(int rate, int channels, int buffer, int flags)
{
    (void)channels;
    (void)buffer;
    (void)flags;

    if (rate <= 0)
        rate = 11025;

    if (rate < 5000)
        rate = 5000;
    if (rate > 22050)
        rate = 22050;

    g_fb_sfx_pcspk_saved_port61 = inportb(FB_SFX_MSDOS_SPEAKER_PORT);

    g_fb_sfx_pcspk_base_port61 = (unsigned char)(g_fb_sfx_pcspk_saved_port61 & ~0x03u);
    outportb(FB_SFX_MSDOS_SPEAKER_PORT, g_fb_sfx_pcspk_base_port61);

    g_fb_sfx_pcspk_rate = rate;
    g_fb_sfx_pcspk_ticks_per_sample = (double)UCLOCKS_PER_SEC / (double)rate;
    g_fb_sfx_pcspk_next_tick = 0.0;
    g_fb_sfx_pcspk_active = 1;

    if (__fb_sfx && g_fb_sfx_pcspk_rate > 0)
        __fb_sfx->samplerate = g_fb_sfx_pcspk_rate;

    SFX_DEBUG("msdos_pcspk: initialized rate=%d", g_fb_sfx_pcspk_rate);
    return 0;
}

static void msdos_pcspk_exit(void)
{
    if (!g_fb_sfx_pcspk_active)
        return;

    outportb(FB_SFX_MSDOS_SPEAKER_PORT, g_fb_sfx_pcspk_saved_port61);

    g_fb_sfx_pcspk_active = 0;
    g_fb_sfx_pcspk_rate = 0;
    g_fb_sfx_pcspk_ticks_per_sample = 0.0;
    g_fb_sfx_pcspk_next_tick = 0.0;
    g_fb_sfx_pcspk_base_port61 = 0;

    SFX_DEBUG("msdos_pcspk: shutdown");
}

static int msdos_pcspk_write(const float *samples, int frames)
{
    int i;
    int channels;

    if (!samples || frames <= 0 || !g_fb_sfx_pcspk_active)
        return -1;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : 2;

    for (i = 0; i < frames; ++i)
    {
        float mixed = 0.0f;
        int c;

        for (c = 0; c < channels; ++c)
            mixed += samples[(i * channels) + c];

        mixed /= (float)channels;

        if (mixed > 1.0f)
            mixed = 1.0f;
        if (mixed < -1.0f)
            mixed = -1.0f;

        fb_sfxPcSpeakerWriteLevel(mixed > 0.0f);
        fb_sfxPcSpeakerWaitForNextSample();
    }

    return frames;
}

#else

static int msdos_pcspk_init(int rate, int channels, int buffer, int flags)
{
    (void)rate;
    (void)channels;
    (void)buffer;
    (void)flags;
    return -1;
}

static void msdos_pcspk_exit(void)
{
}

static int msdos_pcspk_write(const float *samples, int frames)
{
    (void)samples;
    (void)frames;
    return -1;
}

#endif

const FB_SFX_DRIVER fb_sfxDriverPcSpeaker =
{
    "PCSpeaker",
    FB_SFX_DRIVER_CAP_BLOCKING,
    msdos_pcspk_init,
    msdos_pcspk_exit,
    msdos_pcspk_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif
