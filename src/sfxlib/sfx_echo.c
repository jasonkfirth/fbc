/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_echo.c

    Purpose:

        Implement a small global stereo echo for synthesized and sampled
        audio mixed by sfxlib.

    Responsibilities:

        - own and clear the stereo delay line
        - apply a bounded ping-pong feedback delay
        - validate public effect settings before allocating memory

    This file intentionally does NOT contain:

        - oscillator or envelope processing
        - voice allocation
        - platform audio driver code
        - room simulation or convolution reverb
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

/* 16 MB for stereo float samples, far beyond a two-second 384 kHz delay. */
#define FB_SFX_ECHO_MAX_DELAY_FRAMES 2000000

/* ------------------------------------------------------------------------- */
/* Effect state                                                              */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFX_ECHO_STATE
{
    float *samples;
    int frames;
    int position;
    float wet;
    float feedback;
    int enabled;
} FB_SFX_ECHO_STATE;

static FB_SFX_ECHO_STATE g_echo;

/* ------------------------------------------------------------------------- */
/* Lifetime                                                                  */
/* ------------------------------------------------------------------------- */

void fb_sfxEchoInit(void)
{
    memset(&g_echo, 0, sizeof(g_echo));
}

void fb_sfxEchoShutdown(void)
{
    free(g_echo.samples);
    memset(&g_echo, 0, sizeof(g_echo));
}

/* ------------------------------------------------------------------------- */
/* Public effect controls                                                    */
/* ------------------------------------------------------------------------- */

int fb_sfxEchoCmd(float wet, float delay_seconds, float feedback)
{
    float *new_samples;
    double requested_frames;
    int delay_frames;
    size_t delay_samples;

    /* Reject NaN without depending on a platform-specific isfinite macro. */
    if (wet != wet || delay_seconds != delay_seconds || feedback != feedback)
        return -1;

    if (wet < 0.0f || wet > 1.0f ||
        delay_seconds < 0.01f || delay_seconds > 2.0f ||
        feedback < 0.0f || feedback > 0.95f)
    {
        return -1;
    }

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();

    if (!__fb_sfx || __fb_sfx->samplerate <= 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    if (wet == 0.0f)
    {
        fb_sfxEchoShutdown();
        fb_sfxRuntimeUnlock();
        return 0;
    }

    requested_frames = (double)delay_seconds * (double)__fb_sfx->samplerate;
    if (requested_frames < 1.0 ||
        requested_frames > (double)FB_SFX_ECHO_MAX_DELAY_FRAMES)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    delay_frames = (int)(requested_frames + 0.5);

    if ((size_t)delay_frames > ((size_t)-1) / (2u * sizeof(float)))
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    delay_samples = (size_t)delay_frames * 2u;
    new_samples = (float *)calloc(delay_samples, sizeof(float));
    if (!new_samples)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    free(g_echo.samples);
    g_echo.samples = new_samples;
    g_echo.frames = delay_frames;
    g_echo.position = 0;
    g_echo.wet = wet;
    g_echo.feedback = feedback;
    g_echo.enabled = 1;

    fb_sfxRuntimeUnlock();
    return 0;
}

void fb_sfxEchoReset(void)
{
    fb_sfxRuntimeLock();
    fb_sfxEchoShutdown();
    fb_sfxRuntimeUnlock();
}

int fb_sfxEchoEnabled(void)
{
    int enabled;

    fb_sfxRuntimeLock();
    enabled = g_echo.enabled;
    fb_sfxRuntimeUnlock();

    return enabled;
}

/* ------------------------------------------------------------------------- */
/* Mixer processing                                                          */
/* ------------------------------------------------------------------------- */

void fb_sfxEchoProcess(float *left, float *right)
{
    float delayed_left;
    float delayed_right;
    float input_left;
    float input_right;
    int sample_index;

    if (!left || !right || !g_echo.enabled || !g_echo.samples ||
        g_echo.frames <= 0)
    {
        return;
    }

    /*
        The mixer calls this function while holding the runtime lock. The
        opposite-channel feedback creates the stereo bounce generally called
        a ping-pong echo, while the 0.95 feedback limit prevents an unstable
        delay line from growing without bound.
    */
    sample_index = g_echo.position * 2;
    delayed_left = g_echo.samples[sample_index];
    delayed_right = g_echo.samples[sample_index + 1];
    input_left = *left;
    input_right = *right;

    g_echo.samples[sample_index] =
        input_left + (delayed_right * g_echo.feedback);
    g_echo.samples[sample_index + 1] =
        input_right + (delayed_left * g_echo.feedback);

    *left = input_left + (delayed_left * g_echo.wet);
    *right = input_right + (delayed_right * g_echo.wet);

    g_echo.position++;
    if (g_echo.position >= g_echo.frames)
        g_echo.position = 0;
}

void fb_sfxEchoProcessBlock(float *buffer, int frames)
{
    float feedback;
    float wet;
    int delay_frames;
    int frame;
    int position;

    if (!buffer || frames <= 0 || !g_echo.enabled || !g_echo.samples ||
        g_echo.frames <= 0)
    {
        return;
    }

    /*
        Echo feedback makes successive frames dependent, so vectorizing the
        time axis would change the effect.  A block loop still removes two
        pointer checks, several state loads, and one function call per frame
        while preserving the established sample order exactly.
    */
    delay_frames = g_echo.frames;
    position = g_echo.position;
    wet = g_echo.wet;
    feedback = g_echo.feedback;

    for (frame = 0; frame < frames; frame++)
    {
        int delay_index = position * 2;
        int sample_index = frame * 2;
        float delayed_left = g_echo.samples[delay_index];
        float delayed_right = g_echo.samples[delay_index + 1];
        float input_left = buffer[sample_index];
        float input_right = buffer[sample_index + 1];

        g_echo.samples[delay_index] =
            input_left + delayed_right * feedback;
        g_echo.samples[delay_index + 1] =
            input_right + delayed_left * feedback;
        buffer[sample_index] = input_left + delayed_left * wet;
        buffer[sample_index + 1] = input_right + delayed_right * wet;

        position++;
        if (position >= delay_frames)
            position = 0;
    }

    g_echo.position = position;
}

/* end of sfx_echo.c */
