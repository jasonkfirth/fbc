/*
    DOS PC speaker fallback driver.

    This backend uses PIT channel 2 one-shot pulses to approximate
    sample playback on the internal speaker. It is intentionally
    simple and heavily bandwidth-limited, but it gives DOS builds
    a real fallback backend when BLASTER is not available.
*/

#ifndef DISABLE_MSDOS

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"

#include <time.h>

#ifdef __DJGPP__
#include <dos.h>
#endif

#define FB_SFX_MSDOS_PIT_HZ         1193182L
#define FB_SFX_MSDOS_PIT_MODE       0x43
#define FB_SFX_MSDOS_PIT_CH2        0x42
#define FB_SFX_MSDOS_SPEAKER_PORT   0x61

static int g_fb_sfx_pcspk_active = 0;
static int g_fb_sfx_pcspk_rate = 0;
static unsigned int g_fb_sfx_pcspk_max_pulse = 0;
static double g_fb_sfx_pcspk_ticks_per_sample = 0.0;
static double g_fb_sfx_pcspk_next_tick = 0.0;
static unsigned char g_fb_sfx_pcspk_saved_port61 = 0;

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

static void fb_sfxPcSpeakerWritePulse(unsigned int pulse)
{
    if (pulse == 0)
        pulse = 1;

    outportb(FB_SFX_MSDOS_PIT_CH2, (unsigned char)(pulse & 0xFFu));
    outportb(FB_SFX_MSDOS_PIT_CH2, (unsigned char)((pulse >> 8) & 0xFFu));
}

static int msdos_pcspk_init(int rate, int channels, int buffer, int flags)
{
    unsigned long period_counts;

    (void)channels;
    (void)buffer;
    (void)flags;

    if (rate <= 0)
        rate = 11025;

    if (rate < 5000)
        rate = 5000;
    if (rate > 22050)
        rate = 22050;

    period_counts = (unsigned long)(FB_SFX_MSDOS_PIT_HZ / (long)rate);
    if (period_counts == 0)
        period_counts = 1;
    if (period_counts > 65535UL)
        period_counts = 65535UL;

    g_fb_sfx_pcspk_saved_port61 = inportb(FB_SFX_MSDOS_SPEAKER_PORT);

    /* PIT channel 2, lobyte/hibyte, mode 0 one-shot, binary counting. */
    outportb(FB_SFX_MSDOS_PIT_MODE, 0xB0);
    outportb(FB_SFX_MSDOS_SPEAKER_PORT, (unsigned char)(g_fb_sfx_pcspk_saved_port61 | 0x03u));

    g_fb_sfx_pcspk_rate = rate;
    g_fb_sfx_pcspk_max_pulse = (unsigned int)period_counts;
    g_fb_sfx_pcspk_ticks_per_sample = (double)UCLOCKS_PER_SEC / (double)rate;
    g_fb_sfx_pcspk_next_tick = 0.0;
    g_fb_sfx_pcspk_active = 1;

    SFX_DEBUG("msdos_pcspk: initialized rate=%d pulse_max=%u",
              g_fb_sfx_pcspk_rate,
              g_fb_sfx_pcspk_max_pulse);
    return 0;
}

static void msdos_pcspk_exit(void)
{
    if (!g_fb_sfx_pcspk_active)
        return;

    outportb(FB_SFX_MSDOS_SPEAKER_PORT, g_fb_sfx_pcspk_saved_port61);

    g_fb_sfx_pcspk_active = 0;
    g_fb_sfx_pcspk_rate = 0;
    g_fb_sfx_pcspk_max_pulse = 0;
    g_fb_sfx_pcspk_ticks_per_sample = 0.0;
    g_fb_sfx_pcspk_next_tick = 0.0;

    SFX_DEBUG("msdos_pcspk: shutdown");
}

static int msdos_pcspk_write(const float *samples, int frames)
{
    int i;
    int channels;

    if (!samples || frames <= 0 || !g_fb_sfx_pcspk_active || g_fb_sfx_pcspk_max_pulse == 0)
        return -1;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : 2;

    for (i = 0; i < frames; ++i)
    {
        float mixed = 0.0f;
        int c;
        unsigned int sample_u8;
        unsigned int pulse;

        for (c = 0; c < channels; ++c)
            mixed += samples[(i * channels) + c];

        mixed /= (float)channels;

        if (mixed > 1.0f)
            mixed = 1.0f;
        if (mixed < -1.0f)
            mixed = -1.0f;

        sample_u8 = (unsigned int)((mixed + 1.0f) * 127.5f);
        if (sample_u8 > 255u)
            sample_u8 = 255u;

        pulse = (sample_u8 * g_fb_sfx_pcspk_max_pulse) / 255u;
        if (pulse == 0)
            pulse = 1;

        fb_sfxPcSpeakerWritePulse(pulse);
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
    0,
    msdos_pcspk_init,
    msdos_pcspk_exit,
    msdos_pcspk_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif
