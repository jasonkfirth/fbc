/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_pulse.c

    Purpose:

        Implement a PulseAudio backend for sfxlib.
*/

#ifndef DISABLE_LINUX

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_linux.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <stdio.h>
#include <stdlib.h>

static pa_simple *pulse_stream = NULL;
static int pulse_initialized = 0;
static int pulse_debug_write_counter = 0;

static int pulse_debug_enabled(void)
{
    const char *e = getenv("SFXLIB_PULSE_DEBUG");
    return (e && *e && *e != '0');
}

#define PULSE_DBG(...) \
    do { if (pulse_debug_enabled()) fprintf(stderr,"SFX_PULSE: " __VA_ARGS__); } while (0)

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

static float pulse_buffer_peak(const float *buffer, int samples)
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

static int pulse_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    pa_sample_spec spec;
    int error;

    (void)flags;

    if (pulse_initialized)
        return 0;

    if (fb_sfxLinuxInit() != 0)
        return -1;

    fb_sfx_linux.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    fb_sfx_linux.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    fb_sfx_linux.buffer_frames = (buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER;

    PULSE_DBG("initializing PulseAudio driver\n");

    spec.format = PA_SAMPLE_S16LE;
    spec.rate = (unsigned int)fb_sfx_linux.sample_rate;
    spec.channels = (unsigned char)fb_sfx_linux.channels;

    pulse_stream = pa_simple_new(
        NULL,
        "FreeBASIC sfxlib",
        PA_STREAM_PLAYBACK,
        NULL,
        "audio playback",
        &spec,
        NULL,
        NULL,
        &error
    );

    if (!pulse_stream)
    {
        PULSE_DBG("pa_simple_new failed: %s\n", pa_strerror(error));
        return -1;
    }

    if (fb_sfxLinuxActivate(fb_sfx_linux.sample_rate,
                            fb_sfx_linux.channels,
                            fb_sfx_linux.buffer_frames) != 0)
    {
        pa_simple_free(pulse_stream);
        pulse_stream = NULL;
        return -1;
    }

    pulse_initialized = 1;
    pulse_debug_write_counter = 0;

    PULSE_DBG("PulseAudio initialized (rate=%d channels=%d)\n",
              fb_sfx_linux.sample_rate,
              fb_sfx_linux.channels);

    return 0;
}

static void pulse_driver_exit(void)
{
    if (!pulse_initialized)
        return;

    PULSE_DBG("shutting down PulseAudio driver\n");

    if (pulse_stream)
    {
        pa_simple_drain(pulse_stream, NULL);
        pa_simple_free(pulse_stream);
        pulse_stream = NULL;
    }

    pulse_initialized = 0;
    fb_sfxLinuxDeactivate();
}

static int pulse_driver_write(const float *buffer, int frames)
{
    int error;
    int channels;
    int samples;
    short *pcm;

    if (!pulse_stream || !buffer || frames <= 0)
        return -1;

    channels = (fb_sfx_linux.channels > 0) ? fb_sfx_linux.channels : FB_SFX_DEFAULT_CHANNELS;
    samples = frames * channels;

    if (pulse_debug_enabled() && pulse_debug_write_counter < 8)
    {
        PULSE_DBG("write frames=%d samples=%d peak=%0.5f\n",
                  frames,
                  samples,
                  pulse_buffer_peak(buffer, samples));
        pulse_debug_write_counter++;
    }

    pcm = (short *)malloc((size_t)samples * sizeof(short));
    if (!pcm)
        return -1;

    convert_float_to_s16(buffer, pcm, samples);

    if (pa_simple_write(pulse_stream, pcm, (size_t)samples * sizeof(short), &error) < 0)
    {
        PULSE_DBG("write failed: %s\n", pa_strerror(error));
        free(pcm);
        return -1;
    }

    free(pcm);
    return frames;
}

const FB_SFX_DRIVER fb_sfxDriverPulse =
{
    "PulseAudio",
    0,
    pulse_driver_init,
    pulse_driver_exit,
    pulse_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif

/* end of sfx_driver_pulse.c */
