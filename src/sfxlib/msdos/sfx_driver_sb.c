/*
    Very small DOS Sound Blaster playback driver.

    This first version is intentionally simple: it parses BLASTER,
    resets the DSP, and feeds samples through the DSP direct-DAC path.
    It is blocking and not yet a DMA mixer, but it gives the DOS side
    a real hardware-facing starting point.
*/

#ifndef DISABLE_MSDOS

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_msdos.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef __DJGPP__
#include <dos.h>
#endif

static FB_SFX_MSDOS_CONFIG g_fb_sfx_msdos;

int fb_sfxMsdosParseBlaster(FB_SFX_MSDOS_CONFIG *config)
{
    const char *env;
    const char *p;

    if (!config)
        return -1;

    memset(config, 0, sizeof(*config));
    config->base_port = 0x220;
    config->irq = 5;
    config->dma8 = 1;
    config->dma16 = 5;
    config->mpu_port = 0x330;
    config->card_type = 0;

    env = getenv("BLASTER");
    if (!env || !*env)
    {
        config->valid = 1;
        return 0;
    }

    for (p = env; *p; )
    {
        while (*p == ' ' || *p == '\t')
            ++p;

        switch (toupper((unsigned char)*p))
        {
            case 'A':
                config->base_port = (int)strtol(p + 1, NULL, 16);
                break;
            case 'I':
                config->irq = (int)strtol(p + 1, NULL, 10);
                break;
            case 'D':
                config->dma8 = (int)strtol(p + 1, NULL, 10);
                break;
            case 'H':
                config->dma16 = (int)strtol(p + 1, NULL, 10);
                break;
            case 'P':
                config->mpu_port = (int)strtol(p + 1, NULL, 16);
                break;
            case 'T':
                config->card_type = (int)strtol(p + 1, NULL, 10);
                break;
            default:
                break;
        }

        while (*p && *p != ' ' && *p != '\t')
            ++p;
    }

    config->valid = 1;
    return 0;
}

#ifdef __DJGPP__

static int fb_sfxMsdosDspWriteReady(int base_port)
{
    int timeout = 65535;

    while (timeout-- > 0)
    {
        if ((inportb(base_port + 0x0C) & 0x80) == 0)
            return 1;
    }

    return 0;
}

static int fb_sfxMsdosDspReadReady(int base_port)
{
    int timeout = 65535;

    while (timeout-- > 0)
    {
        if (inportb(base_port + 0x0E) & 0x80)
            return 1;
    }

    return 0;
}

static int fb_sfxMsdosDspWrite(int base_port, unsigned char value)
{
    if (!fb_sfxMsdosDspWriteReady(base_port))
        return -1;

    outportb(base_port + 0x0C, value);
    return 0;
}

static int fb_sfxMsdosResetDsp(int base_port)
{
    outportb(base_port + 0x06, 1);
    delay(3);
    outportb(base_port + 0x06, 0);

    if (!fb_sfxMsdosDspReadReady(base_port))
        return -1;

    return (inportb(base_port + 0x0A) == 0xAA) ? 0 : -1;
}

static int msdos_sb_init(int rate, int channels, int buffer, int flags)
{
    (void)rate;
    (void)channels;
    (void)buffer;
    (void)flags;

    if (fb_sfxMsdosParseBlaster(&g_fb_sfx_msdos) != 0)
        return -1;

    return fb_sfxMsdosResetDsp(g_fb_sfx_msdos.base_port);
}

static void msdos_sb_exit(void)
{
    if (g_fb_sfx_msdos.valid)
        fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, 0xD3);
}

static int msdos_sb_write(const float *samples, int frames)
{
    int i;
    int channels;

    if (!samples || frames <= 0 || !g_fb_sfx_msdos.valid)
        return -1;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : 2;

    for (i = 0; i < frames; ++i)
    {
        float mixed = 0.0f;
        int c;
        int sample_u8;

        for (c = 0; c < channels; ++c)
            mixed += samples[(i * channels) + c];

        mixed /= (float)channels;

        if (mixed > 1.0f)
            mixed = 1.0f;
        if (mixed < -1.0f)
            mixed = -1.0f;

        sample_u8 = (int)((mixed + 1.0f) * 127.5f);

        if (fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, 0x10) != 0)
            return -1;
        if (fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, (unsigned char)sample_u8) != 0)
            return -1;
    }

    return frames;
}

#else

static int msdos_sb_init(int rate, int channels, int buffer, int flags)
{
    (void)rate;
    (void)channels;
    (void)buffer;
    (void)flags;
    return -1;
}

static void msdos_sb_exit(void)
{
}

static int msdos_sb_write(const float *samples, int frames)
{
    (void)samples;
    (void)frames;
    return -1;
}

#endif

const FB_SFX_DRIVER fb_sfxDriverSoundBlaster =
{
    "SoundBlaster",
    0,
    msdos_sb_init,
    msdos_sb_exit,
    msdos_sb_write,
    NULL,
    NULL,
    NULL,
    NULL
};

extern const FB_SFX_DRIVER __fb_sfxDriverNull;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverSoundBlaster,
    &__fb_sfxDriverNull,
    NULL
};

#endif
