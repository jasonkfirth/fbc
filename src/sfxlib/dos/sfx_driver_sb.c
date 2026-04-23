/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_sb.c

    Purpose:

        Implement a DOS Sound Blaster playback driver.

        This backend uses the classic Sound Blaster DSP and the ISA
        DMA controller to move 8-bit PCM blocks to the card.

    Responsibilities:

        • parse BLASTER settings
        • reset and program the DSP
        • allocate a DMA-safe DOS buffer
        • transfer mixed PCM blocks through 8-bit DMA playback

    This file intentionally does NOT contain:

        • software mixing logic
        • MIDI playback logic
        • PC speaker fallback playback
        • a full IRQ-driven auto-init mixer

    Design note:

        This driver still performs synchronous block playback.  The
        important improvement over the original direct-DAC path is that
        each write submits a whole DMA block instead of pushing one DSP
        sample at a time.  That greatly reduces CPU overhead and makes
        playback timing less fragile even before a future IRQ-refill
        design is introduced.
*/

#ifndef DISABLE_MSDOS

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_msdos.h"

#include <ctype.h>
#include <dpmi.h>
#include <go32.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __DJGPP__
#include <dos.h>
#include "../../rtlib/dos/fb_dos.h"
#endif

static FB_SFX_MSDOS_CONFIG g_fb_sfx_msdos;
static int g_fb_sfx_msdos_rate = 0;
static int g_fb_sfx_msdos_dma_channel = -1;
static int g_fb_sfx_msdos_dma_buffer_frames = 0;
static int g_fb_sfx_msdos_dma_buffer_bytes = 0;

int fb_sfxMsdosParseBlaster(FB_SFX_MSDOS_CONFIG *config)
{
    const char *env;
    const char *p;

    if (!config)
        return -1;

    memset(config, 0, sizeof(*config));

    env = getenv("BLASTER");
    if (!env || !*env)
        return 0;

    config->have_blaster = 1;

    for (p = env; *p; )
    {
        while (*p == ' ' || *p == '\t')
            ++p;

        switch (toupper((unsigned char)*p))
        {
            case 'A':
                config->base_port = (int)strtol(p + 1, NULL, 16);
                config->have_base_port = (config->base_port > 0);
                break;
            case 'I':
                config->irq = (int)strtol(p + 1, NULL, 10);
                config->have_irq = (config->irq > 0);
                break;
            case 'D':
                config->dma8 = (int)strtol(p + 1, NULL, 10);
                config->have_dma8 = (config->dma8 >= 0);
                break;
            case 'H':
                config->dma16 = (int)strtol(p + 1, NULL, 10);
                config->have_dma16 = (config->dma16 >= 0);
                break;
            case 'P':
                config->mpu_port = (int)strtol(p + 1, NULL, 16);
                config->have_mpu_port = (config->mpu_port > 0);
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

#define FB_SFX_MSDOS_DMA_8BIT_MAX_BYTES  32768

typedef struct FB_SFX_MSDOS_DMA_BUFFER
{
    int selector;
    unsigned short segment;
    unsigned long linear;
    unsigned long dma_linear;
    unsigned int dma_offset;
    unsigned long page;
    int bytes;
} FB_SFX_MSDOS_DMA_BUFFER;

static FB_SFX_MSDOS_DMA_BUFFER g_fb_sfx_msdos_dma_buffer;
static unsigned char *g_fb_sfx_msdos_mix_buffer = NULL;

static const unsigned short g_fb_sfx_msdos_dma_addr_port[4] =
{
    0x00, 0x02, 0x04, 0x06
};

static const unsigned short g_fb_sfx_msdos_dma_count_port[4] =
{
    0x01, 0x03, 0x05, 0x07
};

static const unsigned short g_fb_sfx_msdos_dma_page_port[4] =
{
    0x87, 0x83, 0x81, 0x82
};

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

static void fb_sfxMsdosWaitForPlaybackTicks(double ticks)
{
    double target_tick;
    uclock_t now;

    if (ticks <= 0.0)
        return;

    target_tick = (double)uclock() + ticks;

    do
    {
        now = uclock();
    } while ((double)now < target_tick);
}

static int fb_sfxMsdosSelectDmaChannel(const FB_SFX_MSDOS_CONFIG *config)
{
    if (!config)
        return -1;

    /*
        This first DMA-backed version only uses the classic 8-bit DMA
        channels.  Sound Blaster 1.x/2.x cards commonly expose channel 1,
        while some setups use channel 3.
    */

    if (config->have_dma8 &&
        (config->dma8 == 0 || config->dma8 == 1 || config->dma8 == 3))
    {
        return config->dma8;
    }

    return -1;
}

static int fb_sfxMsdosAllocDmaBuffer(int bytes)
{
    int paragraphs;
    int selector;
    int segment;
    unsigned long linear;
    unsigned long dma_linear;
    unsigned long page_end;

    if (bytes <= 0)
        return -1;

    memset(&g_fb_sfx_msdos_dma_buffer, 0, sizeof(g_fb_sfx_msdos_dma_buffer));

    /*
        ISA DMA channels 0-3 cannot cross a 64 KiB physical boundary.
        Allocate extra conventional memory and align the active window
        so the selected playback block fits entirely inside one DMA page.
    */

    paragraphs = (bytes + 65535 + 15) >> 4;
    segment = __dpmi_allocate_dos_memory(paragraphs, &selector);
    if (segment == 0)
        return -1;

    linear = ((unsigned long)segment) << 4;
    dma_linear = (linear + 0xFFFFUL) & ~0xFFFFUL;
    if ((dma_linear - linear) + (unsigned long)bytes > (unsigned long)(paragraphs << 4))
    {
        __dpmi_free_dos_memory(selector);
        return -1;
    }

    page_end = (dma_linear & ~0xFFFFUL) + 0x10000UL;
    if (dma_linear + (unsigned long)bytes > page_end)
    {
        __dpmi_free_dos_memory(selector);
        return -1;
    }

    g_fb_sfx_msdos_dma_buffer.selector = selector;
    g_fb_sfx_msdos_dma_buffer.segment = (unsigned short)segment;
    g_fb_sfx_msdos_dma_buffer.linear = linear;
    g_fb_sfx_msdos_dma_buffer.dma_linear = dma_linear;
    g_fb_sfx_msdos_dma_buffer.dma_offset = (unsigned int)(dma_linear & 0xFFFFUL);
    g_fb_sfx_msdos_dma_buffer.page = (dma_linear >> 16) & 0xFFUL;
    g_fb_sfx_msdos_dma_buffer.bytes = bytes;

    return 0;
}

static void fb_sfxMsdosFreeDmaBuffer(void)
{
    if (g_fb_sfx_msdos_dma_buffer.selector != 0)
        __dpmi_free_dos_memory(g_fb_sfx_msdos_dma_buffer.selector);

    memset(&g_fb_sfx_msdos_dma_buffer, 0, sizeof(g_fb_sfx_msdos_dma_buffer));
}

static int fb_sfxMsdosSetSampleRate(int base_port, int rate)
{
    int clamped_rate;
    int time_constant;

    clamped_rate = rate;
    if (clamped_rate < 4000)
        clamped_rate = 4000;
    if (clamped_rate > 22050)
        clamped_rate = 22050;

    time_constant = 256 - (1000000 / clamped_rate);
    if (time_constant < 0)
        time_constant = 0;
    if (time_constant > 255)
        time_constant = 255;

    if (fb_sfxMsdosDspWrite(base_port, 0x40) != 0)
        return -1;
    if (fb_sfxMsdosDspWrite(base_port, (unsigned char)time_constant) != 0)
        return -1;

    g_fb_sfx_msdos_rate = clamped_rate;
    return 0;
}

static int fb_sfxMsdosProgramDma8(int channel, unsigned int offset, unsigned char page, unsigned int count)
{
    if (channel < 0 || channel > 3)
        return -1;

    if (count == 0)
        return -1;

    /*
        8237 DMA programming sequence for an 8-bit memory-to-device transfer:

            1. mask the channel
            2. clear the internal flip-flop
            3. set mode, address, count, and page
            4. unmask the channel

        Sound Blaster 8-bit playback uses single-cycle DMA write mode.
    */

    outportb(0x0A, (unsigned char)(0x04u | (unsigned char)channel));
    outportb(0x0C, 0x00);
    outportb(0x0B, (unsigned char)(0x48u | (unsigned char)channel));
    outportb(g_fb_sfx_msdos_dma_addr_port[channel], (unsigned char)(offset & 0xFFu));
    outportb(g_fb_sfx_msdos_dma_addr_port[channel], (unsigned char)((offset >> 8) & 0xFFu));
    outportb(g_fb_sfx_msdos_dma_page_port[channel], page);
    outportb(g_fb_sfx_msdos_dma_count_port[channel], (unsigned char)((count - 1u) & 0xFFu));
    outportb(g_fb_sfx_msdos_dma_count_port[channel], (unsigned char)(((count - 1u) >> 8) & 0xFFu));
    outportb(0x0A, (unsigned char)channel);

    return 0;
}

static int fb_sfxMsdosStartDmaPlayback(int base_port, unsigned int bytes)
{
    if (bytes == 0 || bytes > 65536u)
        return -1;

    if (fb_sfxMsdosDspWrite(base_port, 0x14) != 0)
        return -1;
    if (fb_sfxMsdosDspWrite(base_port, (unsigned char)((bytes - 1u) & 0xFFu)) != 0)
        return -1;
    if (fb_sfxMsdosDspWrite(base_port, (unsigned char)(((bytes - 1u) >> 8) & 0xFFu)) != 0)
        return -1;

    return 0;
}

static void fb_sfxMsdosConvertToUnsignedMono8(const float *samples, int frames, unsigned char *pcm)
{
    int channels;
    int i;

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
        if (sample_u8 < 0)
            sample_u8 = 0;
        if (sample_u8 > 255)
            sample_u8 = 255;

        pcm[i] = (unsigned char)sample_u8;
    }
}

static int msdos_sb_init(int rate, int channels, int buffer, int flags)
{
    int dma_channel;
    int dma_frames;
    int dma_bytes;

    (void)channels;
    (void)flags;

    if (fb_sfxMsdosParseBlaster(&g_fb_sfx_msdos) != 0)
        return -1;

    if (!g_fb_sfx_msdos.have_blaster || !g_fb_sfx_msdos.have_base_port)
    {
        SFX_DEBUG("msdos_sb: BLASTER is missing or does not specify a base port");
        return -1;
    }

    if (fb_sfxMsdosResetDsp(g_fb_sfx_msdos.base_port) != 0)
    {
        SFX_DEBUG("msdos_sb: DSP reset failed at 0x%X", g_fb_sfx_msdos.base_port);
        return -1;
    }

    dma_channel = fb_sfxMsdosSelectDmaChannel(&g_fb_sfx_msdos);
    if (dma_channel < 0)
    {
        SFX_DEBUG("msdos_sb: no supported 8-bit DMA channel in BLASTER");
        return -1;
    }

    dma_frames = buffer;
    if (dma_frames <= 0)
        dma_frames = 2048;
    if (dma_frames > FB_SFX_MSDOS_DMA_8BIT_MAX_BYTES)
        dma_frames = FB_SFX_MSDOS_DMA_8BIT_MAX_BYTES;

    dma_bytes = dma_frames;
    if (dma_bytes <= 0)
        return -1;

    if (fb_sfxMsdosAllocDmaBuffer(dma_bytes) != 0)
    {
        SFX_DEBUG("msdos_sb: failed to allocate DMA buffer (%d bytes)", dma_bytes);
        return -1;
    }

    g_fb_sfx_msdos_mix_buffer = (unsigned char *)malloc((size_t)dma_bytes);
    if (!g_fb_sfx_msdos_mix_buffer)
    {
        SFX_DEBUG("msdos_sb: failed to allocate staging buffer (%d bytes)", dma_bytes);
        fb_sfxMsdosFreeDmaBuffer();
        return -1;
    }

    if (fb_sfxMsdosSetSampleRate(g_fb_sfx_msdos.base_port, rate) != 0)
    {
        SFX_DEBUG("msdos_sb: failed to program sample rate %d", rate);
        free(g_fb_sfx_msdos_mix_buffer);
        g_fb_sfx_msdos_mix_buffer = NULL;
        fb_sfxMsdosFreeDmaBuffer();
        return -1;
    }

    if (fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, 0xD1) != 0)
    {
        SFX_DEBUG("msdos_sb: failed to enable speaker");
        free(g_fb_sfx_msdos_mix_buffer);
        g_fb_sfx_msdos_mix_buffer = NULL;
        fb_sfxMsdosFreeDmaBuffer();
        return -1;
    }

    g_fb_sfx_msdos_dma_channel = dma_channel;
    g_fb_sfx_msdos_dma_buffer_frames = dma_frames;
    g_fb_sfx_msdos_dma_buffer_bytes = dma_bytes;
    g_fb_sfx_msdos.valid = 1;
    SFX_DEBUG("msdos_sb: initialized at A%X I%d D%d H%d rate=%d dma=%d block=%d",
              g_fb_sfx_msdos.base_port,
              g_fb_sfx_msdos.irq,
              g_fb_sfx_msdos.dma8,
              g_fb_sfx_msdos.dma16,
              g_fb_sfx_msdos_rate,
              g_fb_sfx_msdos_dma_channel,
              g_fb_sfx_msdos_dma_buffer_bytes);
    return 0;
}

static void msdos_sb_exit(void)
{
    if (g_fb_sfx_msdos.valid)
    {
        fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, 0xD0);
        fb_sfxMsdosDspWrite(g_fb_sfx_msdos.base_port, 0xD3);
    }

    g_fb_sfx_msdos.valid = 0;
    g_fb_sfx_msdos_rate = 0;
    g_fb_sfx_msdos_dma_channel = -1;
    g_fb_sfx_msdos_dma_buffer_frames = 0;
    g_fb_sfx_msdos_dma_buffer_bytes = 0;
    free(g_fb_sfx_msdos_mix_buffer);
    g_fb_sfx_msdos_mix_buffer = NULL;
    fb_sfxMsdosFreeDmaBuffer();
}

static int msdos_sb_write(const float *samples, int frames)
{
    int written;

    if (!samples || frames <= 0 || !g_fb_sfx_msdos.valid)
        return -1;

    written = 0;

    while (written < frames)
    {
        double playback_ticks;
        int chunk_frames;
        int chunk_bytes;
        int linear_offset;

        chunk_frames = frames - written;
        if (chunk_frames > g_fb_sfx_msdos_dma_buffer_frames)
            chunk_frames = g_fb_sfx_msdos_dma_buffer_frames;

        chunk_bytes = chunk_frames;
        linear_offset = (int)(g_fb_sfx_msdos_dma_buffer.dma_linear -
                              g_fb_sfx_msdos_dma_buffer.linear);

        fb_sfxMsdosConvertToUnsignedMono8(samples + (written * ((__fb_sfx && __fb_sfx->output_channels > 0)
            ? __fb_sfx->output_channels
            : 2)),
            chunk_frames,
            g_fb_sfx_msdos_mix_buffer);

        movedata(_go32_my_ds(),
                 (unsigned)g_fb_sfx_msdos_mix_buffer,
                 g_fb_sfx_msdos_dma_buffer.selector,
                 linear_offset,
                 (unsigned)chunk_bytes);

        if (fb_sfxMsdosProgramDma8(g_fb_sfx_msdos_dma_channel,
                                   g_fb_sfx_msdos_dma_buffer.dma_offset,
                                   (unsigned char)g_fb_sfx_msdos_dma_buffer.page,
                                   (unsigned int)chunk_bytes) != 0)
        {
            return -1;
        }

        if (fb_sfxMsdosStartDmaPlayback(g_fb_sfx_msdos.base_port, (unsigned int)chunk_bytes) != 0)
            return -1;

        playback_ticks = ((double)UCLOCKS_PER_SEC * (double)chunk_frames) /
                         (double)g_fb_sfx_msdos_rate;
        fb_sfxMsdosWaitForPlaybackTicks(playback_ticks);

        /*
            Reading the DSP status port acknowledges completion on the
            classic 8-bit DMA path and leaves the DSP ready for the next
            block command.
        */
        (void)inportb(g_fb_sfx_msdos.base_port + 0x0E);

        written += chunk_frames;
    }

    return written;
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

extern const FB_SFX_DRIVER fb_sfxDriverPcSpeaker;
extern const FB_SFX_DRIVER __fb_sfxDriverNull;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverSoundBlaster,
    &fb_sfxDriverPcSpeaker,
    &__fb_sfxDriverNull,
    NULL
};

#endif
