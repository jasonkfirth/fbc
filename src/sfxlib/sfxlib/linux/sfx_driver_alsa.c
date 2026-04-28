/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_alsa.c

    Purpose:

        Implement an ALSA audio driver for Linux.
*/

#ifndef DISABLE_LINUX

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_linux.h"

#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>

static snd_pcm_t *alsa_pcm = NULL;
static int alsa_initialized = 0;
static int alsa_debug_write_counter = 0;

static int alsa_env_enabled(const char *name)
{
    const char *e = getenv(name);
    return (e && *e && *e != '0');
}

static int alsa_debug_enabled(void)
{
    return alsa_env_enabled("SFXLIB_ALSA_DEBUG");
}

static int alsa_probe_noise_enabled(void)
{
    return alsa_env_enabled("SFXLIB_DEBUG") ||
           alsa_env_enabled("SFXLIB_LINUX_DEBUG") ||
           alsa_env_enabled("SFXLIB_ALSA_DEBUG");
}

static void alsa_silent_error_handler(const char *file,
                                      int line,
                                      const char *function,
                                      int err,
                                      const char *fmt,
                                      ...)
{
    (void)file;
    (void)line;
    (void)function;
    (void)err;
    (void)fmt;
}

#define ALSA_DBG(...) \
    do { if (alsa_debug_enabled()) fprintf(stderr, "SFX_ALSA: " __VA_ARGS__); } while (0)

static void convert_float_to_s16(const float *in, short *out, int samples)
{
    int i;

    for (i = 0; i < samples; ++i)
    {
        float v = in[i];

        if (v > 1.0f)
            v = 1.0f;
        else if (v < -1.0f)
            v = -1.0f;

        out[i] = (short)(v * 32767.0f);
    }
}

static float alsa_buffer_peak(const float *buffer, int samples)
{
    float peak = 0.0f;
    int i;

    for (i = 0; i < samples; ++i)
    {
        float v = buffer[i];

        if (v < 0.0f)
            v = -v;

        if (v > peak)
            peak = v;
    }

    return peak;
}

static int alsa_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    int err;
    int quiet_errors;
    snd_pcm_hw_params_t *params;
    snd_lib_error_handler_t previous_error_handler = NULL;
    unsigned int actual_rate;
    snd_pcm_uframes_t actual_buffer_frames;
    int dir = 0;

    (void)flags;

    if (alsa_initialized)
        return 0;

    if (fb_sfxLinuxInit() != 0)
        return -1;

    fb_sfx_linux.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    fb_sfx_linux.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    fb_sfx_linux.buffer_frames = (buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER;

    actual_rate = (unsigned int)fb_sfx_linux.sample_rate;
    actual_buffer_frames = (snd_pcm_uframes_t)(fb_sfx_linux.buffer_frames * 4);

    ALSA_DBG("initializing ALSA driver\n");

    quiet_errors = !alsa_probe_noise_enabled();
    if (quiet_errors)
    {
        previous_error_handler = snd_lib_error;
        snd_lib_error_set_handler(alsa_silent_error_handler);
    }

    err = snd_pcm_open(&alsa_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        if (quiet_errors)
            snd_lib_error_set_handler(previous_error_handler);
        ALSA_DBG("snd_pcm_open failed: %s\n", snd_strerror(err));
        return -1;
    }

    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(alsa_pcm, params);
    snd_pcm_hw_params_set_access(alsa_pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(alsa_pcm, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(alsa_pcm, params, (unsigned int)fb_sfx_linux.channels);
    snd_pcm_hw_params_set_rate_near(alsa_pcm, params, &actual_rate, &dir);
    snd_pcm_hw_params_set_buffer_size_near(alsa_pcm, params, &actual_buffer_frames);

    err = snd_pcm_hw_params(alsa_pcm, params);
    if (err < 0)
    {
        if (quiet_errors)
            snd_lib_error_set_handler(previous_error_handler);
        ALSA_DBG("snd_pcm_hw_params failed: %s\n", snd_strerror(err));
        snd_pcm_close(alsa_pcm);
        alsa_pcm = NULL;
        return -1;
    }

    snd_pcm_prepare(alsa_pcm);

    if (fb_sfxLinuxActivate((int)actual_rate,
                            fb_sfx_linux.channels,
                            (int)actual_buffer_frames) != 0)
    {
        if (quiet_errors)
            snd_lib_error_set_handler(previous_error_handler);
        snd_pcm_close(alsa_pcm);
        alsa_pcm = NULL;
        return -1;
    }

    if (quiet_errors)
        snd_lib_error_set_handler(previous_error_handler);

    alsa_initialized = 1;
    alsa_debug_write_counter = 0;

    ALSA_DBG("ALSA initialized (rate=%u channels=%d buffer=%lu)\n",
             actual_rate,
             fb_sfx_linux.channels,
             (unsigned long)actual_buffer_frames);

    return 0;
}

static void alsa_driver_exit(void)
{
    if (!alsa_initialized)
        return;

    ALSA_DBG("shutting down ALSA driver\n");

    if (alsa_pcm)
    {
        snd_pcm_drain(alsa_pcm);
        snd_pcm_close(alsa_pcm);
        alsa_pcm = NULL;
    }

    fb_sfxLinuxDeactivate();
    alsa_initialized = 0;
}

static int alsa_driver_write(const float *buffer, int frames)
{
    short *pcm;
    int channels;
    int samples;
    int err;

    if (!alsa_pcm || !buffer || frames <= 0)
        return -1;

    channels = (fb_sfx_linux.channels > 0) ? fb_sfx_linux.channels : FB_SFX_DEFAULT_CHANNELS;
    samples = frames * channels;

    if (alsa_debug_enabled() && alsa_debug_write_counter < 8)
    {
        ALSA_DBG("write frames=%d samples=%d peak=%0.5f\n",
                 frames,
                 samples,
                 alsa_buffer_peak(buffer, samples));
        alsa_debug_write_counter++;
    }

    pcm = (short *)malloc((size_t)samples * sizeof(short));
    if (!pcm)
        return -1;

    convert_float_to_s16(buffer, pcm, samples);

    err = (int)snd_pcm_writei(alsa_pcm, pcm, (snd_pcm_uframes_t)frames);
    free(pcm);

    if (err == -EPIPE)
    {
        ALSA_DBG("underrun detected\n");
        snd_pcm_prepare(alsa_pcm);
        return 0;
    }

    if (err < 0)
    {
        ALSA_DBG("write error: %s\n", snd_strerror(err));
        return -1;
    }

    return err;
}

const FB_SFX_DRIVER fb_sfxDriverAlsa =
{
    "ALSA",
    FB_SFX_DRIVER_CAP_CAPTURE | FB_SFX_DRIVER_CAP_MIDI,
    alsa_driver_init,
    alsa_driver_exit,
    alsa_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif

/* end of sfx_driver_alsa.c */
