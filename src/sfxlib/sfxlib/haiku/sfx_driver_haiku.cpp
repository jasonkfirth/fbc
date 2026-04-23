#ifndef DISABLE_HAIKU

#include "fb_sfx_haiku.h"

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"

#include <SoundPlayer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

FB_SFX_HAIKU_STATE fb_sfx_haiku =
{
    0,
    44100,
    2,
    1024,
    0
};

/* ------------------------------------------------------------------------- */
/* Ring buffer                                                               */
/* ------------------------------------------------------------------------- */

static float *rb_data = NULL;
static int rb_size = 0;
static volatile int rb_write = 0;
static volatile int rb_read = 0;

static void rb_shutdown(void)
{
    if (rb_data)
    {
        free(rb_data);
        rb_data = NULL;
    }

    rb_size = 0;
    rb_write = 0;
    rb_read = 0;
}

static int rb_init(int frames, int channels)
{
    rb_size = frames * channels * 4; /* extra slack */
    rb_data = (float*)malloc(rb_size * sizeof(float));
    if (!rb_data) return -1;
    rb_write = rb_read = 0;
    return 0;
}

static int rb_available(void)
{
    int w = rb_write;
    int r = rb_read;
    return (w >= r) ? (w - r) : (rb_size - (r - w));
}

static void rb_push(const float *in, int count)
{
    for (int i = 0; i < count; i++)
    {
        rb_data[rb_write] = in[i];
        rb_write = (rb_write + 1) % rb_size;
        if (rb_write == rb_read) /* overwrite oldest */
            rb_read = (rb_read + 1) % rb_size;
    }
}

static void rb_pop(float *out, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (rb_read == rb_write)
        {
            out[i] = 0.0f;
        }
        else
        {
            out[i] = rb_data[rb_read];
            rb_read = (rb_read + 1) % rb_size;
        }
    }
}

/* ------------------------------------------------------------------------- */
/* SoundPlayer                                                               */
/* ------------------------------------------------------------------------- */

static BSoundPlayer *g_player = NULL;

static void haiku_player_shutdown(void)
{
    if (g_player)
    {
        g_player->Stop();
        delete g_player;
        g_player = NULL;
    }
}

static void audio_callback(void *cookie, void *buffer, size_t size, const media_raw_audio_format &fmt)
{
    (void)cookie;
    (void)fmt;

    int samples = size / sizeof(int16_t);
    int16_t *out = (int16_t*)buffer;

    float temp[2048];
    if (samples > 2048) samples = 2048;

    rb_pop(temp, samples);

    for (int i = 0; i < samples; i++)
    {
        float v = temp[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        out[i] = (int16_t)(v * 32767.0f);
    }
}

/* ------------------------------------------------------------------------- */
/* Driver core                                                               */
/* ------------------------------------------------------------------------- */

static int haiku_driver_init(int rate, int channels, int buffer, int flags)
{
    status_t err;
    int bytes_per_sample;

    (void)flags;

    if (fb_sfx_haiku.running)
        return 0;

    fb_sfx_haiku.sample_rate = rate > 0 ? rate : 44100;
    fb_sfx_haiku.channels = channels > 0 ? channels : 2;
    fb_sfx_haiku.buffer_frames = buffer > 0 ? buffer : 1024;

    if (rb_init(fb_sfx_haiku.buffer_frames, fb_sfx_haiku.channels) != 0)
        return -1;

    media_raw_audio_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.frame_rate = fb_sfx_haiku.sample_rate;
    fmt.channel_count = fb_sfx_haiku.channels;
    fmt.format = media_raw_audio_format::B_AUDIO_SHORT;
    fmt.byte_order = B_MEDIA_LITTLE_ENDIAN;
    bytes_per_sample = (int)sizeof(int16_t);
    fmt.buffer_size = fb_sfx_haiku.buffer_frames *
                      fb_sfx_haiku.channels *
                      bytes_per_sample;

    if (fmt.buffer_size <= 0)
    {
        rb_shutdown();
        return -1;
    }

    g_player = new BSoundPlayer(&fmt, "fbsfx", audio_callback, NULL);
    if (!g_player)
    {
        rb_shutdown();
        return -1;
    }

    err = g_player->InitCheck();
    if (err != B_OK)
    {
        haiku_player_shutdown();
        rb_shutdown();
        return -1;
    }

    g_player->SetHasData(true);
    g_player->Start();

    fb_sfx_haiku.initialized = 1;
    fb_sfx_haiku.running = 1;

    return 0;
}

static void haiku_driver_exit(void)
{
    fb_sfx_haiku.running = 0;
    fb_sfx_haiku.initialized = 0;

    haiku_player_shutdown();
    rb_shutdown();
}

static int haiku_driver_write(const float *buffer, int frames)
{
    if (!fb_sfx_haiku.running || !buffer || frames <= 0)
        return 0;

    int count = frames * fb_sfx_haiku.channels;
    rb_push(buffer, count);

    return frames;
}

static int haiku_driver_capture_read(short *buffer, int frames)
{
    (void)buffer;
    (void)frames;
    return 0;
}

static void haiku_driver_poll(void)
{
}

/* ------------------------------------------------------------------------- */
/* Export                                                                    */
/* ------------------------------------------------------------------------- */

extern "C" const FB_SFX_DRIVER fb_sfxDriverHaiku =
{
    "Haiku",
    0,
    haiku_driver_init,
    haiku_driver_exit,
    haiku_driver_write,
    haiku_driver_capture_read,
    haiku_driver_poll,
    NULL,
    NULL
};

#endif
