/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_alsa.c

    Purpose:

        Provide ALSA-based audio capture for sfxlib.

        This module allows recording from microphone or
        other audio input devices using ALSA PCM capture.

    Responsibilities:

        • initialize ALSA capture device
        • read captured audio frames
        • convert PCM input to float samples
        • feed captured audio into sfxlib buffers

    This file intentionally does NOT contain:

        • mixer logic
        • playback code
        • capture file encoding
*/

#ifndef DISABLE_LINUX

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_linux.h"

#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* ALSA capture state                                                        */
/* ------------------------------------------------------------------------- */

static snd_pcm_t *capture_pcm = NULL;

static int capture_initialized = 0;
static int capture_running = 0;


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int capture_debug_enabled(void)
{
    const char *e = getenv("SFXLIB_CAPTURE_DEBUG");
    return (e && *e && *e != '0');
}

#define CAPTURE_DBG(...) \
    do { if (capture_debug_enabled()) fprintf(stderr,"SFX_CAPTURE: " __VA_ARGS__); } while(0)


/* ------------------------------------------------------------------------- */
/* Float conversion                                                          */
/* ------------------------------------------------------------------------- */

static void convert_s16_to_float(short *in, float *out, int samples)
{
    int i;

    for (i = 0; i < samples; i++)
    {
        out[i] = (float)in[i] / 32768.0f;
    }
}


/* ------------------------------------------------------------------------- */
/* Capture initialization                                                    */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureAlsaInit(void)
{
    int err;
    snd_pcm_hw_params_t *params;
    unsigned int rate;
    int dir = 0;

    if (capture_initialized)
        return 0;

    CAPTURE_DBG("initializing ALSA capture device\n");

    rate = fb_sfx_linux.sample_rate;

    err = snd_pcm_open(
        &capture_pcm,
        "default",
        SND_PCM_STREAM_CAPTURE,
        0
    );

    if (err < 0)
    {
        CAPTURE_DBG("snd_pcm_open failed: %s\n",
                    snd_strerror(err));
        return -1;
    }

    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(capture_pcm, params);

    snd_pcm_hw_params_set_access(
        capture_pcm,
        params,
        SND_PCM_ACCESS_RW_INTERLEAVED
    );

    snd_pcm_hw_params_set_format(
        capture_pcm,
        params,
        SND_PCM_FORMAT_S16_LE
    );

    snd_pcm_hw_params_set_channels(
        capture_pcm,
        params,
        fb_sfx_linux.channels
    );

    snd_pcm_hw_params_set_rate_near(
        capture_pcm,
        params,
        &rate,
        &dir
    );

    snd_pcm_hw_params_set_buffer_size(
        capture_pcm,
        params,
        fb_sfx_linux.buffer_frames * 4
    );

    err = snd_pcm_hw_params(capture_pcm, params);

    if (err < 0)
    {
        CAPTURE_DBG("hw_params failed: %s\n",
                    snd_strerror(err));

        snd_pcm_close(capture_pcm);
        capture_pcm = NULL;

        return -1;
    }

    snd_pcm_prepare(capture_pcm);

    capture_initialized = 1;

    CAPTURE_DBG("capture initialized (rate=%u channels=%d)\n",
                rate,
                fb_sfx_linux.channels);

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Start capture                                                             */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureAlsaStart(void)
{
    if (fb_sfxLinuxInit() != 0)
        return -1;

    if (!capture_initialized)
    {
        if (fb_sfxCaptureAlsaInit() != 0)
            return -1;
    }

    capture_running = 1;

    CAPTURE_DBG("capture started\n");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Stop capture                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxCaptureAlsaStop(void)
{
    if (!capture_initialized)
        return;

    capture_running = 0;
    snd_pcm_drop(capture_pcm);

    CAPTURE_DBG("capture stopped\n");
}


/* ------------------------------------------------------------------------- */
/* Shutdown                                                                  */
/* ------------------------------------------------------------------------- */

void fb_sfxCaptureAlsaShutdown(void)
{
    if (!capture_initialized)
        return;

    CAPTURE_DBG("capture shutdown\n");

    snd_pcm_close(capture_pcm);
    capture_pcm = NULL;

    capture_initialized = 0;
}


/* ------------------------------------------------------------------------- */
/* Read capture frames                                                       */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureAlsaRead(float *buffer, int frames)
{
    short *pcm;
    int err;
    int samples;

    if (!capture_running || !capture_pcm)
        return 0;

    samples = frames * fb_sfx_linux.channels;

    pcm = (short*)malloc(samples * sizeof(short));
    if (!pcm)
        return -1;

    err = snd_pcm_readi(capture_pcm, pcm, frames);

    if (err == -EPIPE)
    {
        CAPTURE_DBG("capture overrun\n");
        snd_pcm_prepare(capture_pcm);
        free(pcm);
        return 0;
    }
    else if (err < 0)
    {
        CAPTURE_DBG("read error: %s\n",
                    snd_strerror(err));
        free(pcm);
        return -1;
    }

    convert_s16_to_float(pcm, buffer, samples);

    free(pcm);

    return err;
}

#endif

/* end of sfx_capture_alsa.c */
