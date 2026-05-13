/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_audio_play.c

    Purpose:

        Implement the AUDIO PLAY command.

        This command plays an external audio file using the
        sfxlib streaming system. Unlike the SFX commands, which
        operate on preloaded sound assets, AUDIO PLAY handles
        general-purpose file playback.

    Responsibilities:

        • open an audio file
        • prepare a streaming playback context
        • feed decoded audio into the sfxlib mixer

    This file intentionally does NOT contain:

        • audio file decoding logic
        • driver playback logic
        • mixer algorithms

    Architectural overview:

        AUDIO PLAY "file"
              │
        stream decoder
              │
        mixer input stream
              │
        platform driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* Stream playback state                                                     */
/* ------------------------------------------------------------------------- */

FILE *g_audio_file = NULL;
float *g_audio_data = NULL;
int   g_audio_frames = 0;
int   g_audio_channels = 0;
int   g_audio_samplerate = 0;
int   g_audio_position = 0;
float g_audio_sample_pos = 0.0f;
float g_audio_sample_step = 1.0f;
int   g_audio_loop = 0;
int   g_audio_playing = 0;
int   g_audio_paused = 0;


/* ------------------------------------------------------------------------- */
/* Stop existing playback                                                    */
/* ------------------------------------------------------------------------- */

extern void fb_sfxAudioStopInternal(void);


/* ------------------------------------------------------------------------- */
/* Sample-rate stepping                                                       */
/* ------------------------------------------------------------------------- */

static float fb_sfxAudioSampleStep(int source_rate)
{
    if (source_rate > 0 && __fb_sfx && __fb_sfx->samplerate > 0)
        return (float)source_rate / (float)__fb_sfx->samplerate;

    return 1.0f;
}

static void fb_sfxAudioReadFrame(float sample_pos, float *left, float *right)
{
    const float *src;
    const float *next;
    float fraction;
    float next_left;
    float next_right;
    int index;
    int next_index;

    if (!g_audio_data || g_audio_frames <= 0 || g_audio_channels <= 0)
    {
        *left = 0.0f;
        *right = 0.0f;
        return;
    }

    index = (int)sample_pos;
    if (index < 0)
        index = 0;
    if (index >= g_audio_frames)
        index = g_audio_frames - 1;

    next_index = index + 1;
    if (next_index >= g_audio_frames)
        next_index = g_audio_loop ? 0 : index;

    fraction = sample_pos - (float)index;
    if (fraction < 0.0f)
        fraction = 0.0f;

    src = g_audio_data + (index * g_audio_channels);
    next = g_audio_data + (next_index * g_audio_channels);

    *left = src[0];
    *right = (g_audio_channels > 1) ? src[1] : src[0];

    next_left = next[0];
    next_right = (g_audio_channels > 1) ? next[1] : next[0];

    *left += (next_left - *left) * fraction;
    *right += (next_right - *right) * fraction;
}


/* ------------------------------------------------------------------------- */
/* AUDIO PLAY                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioPlay()

    Start playback of an audio file.

    Currently this function only opens the file and prepares
    a streaming context. Actual decoding is handled by the
    stream subsystem.
*/

int fb_sfxAudioPlay(const char *filename)
{
    float *decoded = NULL;
    int frames = 0;
    int channels = 0;
    int sample_rate = 0;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();

    if (!filename)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    /*
        Stop any previous playback
    */

    fb_sfxAudioStopInternal();

    if (fb_sfxDecodeFile(filename,
                         &decoded,
                         &frames,
                         &channels,
                         &sample_rate) != 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    g_audio_data = decoded;
    g_audio_frames = frames;
    g_audio_channels = channels;
    g_audio_samplerate = sample_rate;
    g_audio_position = 0;
    g_audio_sample_pos = 0.0f;
    g_audio_sample_step = fb_sfxAudioSampleStep(sample_rate);

    g_audio_loop = 0;
    g_audio_playing = 1;
    g_audio_paused = 0;

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* AUDIO LOOP                                                                */
/* ------------------------------------------------------------------------- */

int fb_sfxAudioLoop(const char *filename)
{
    int result;

    if (!fb_sfxEnsureInitialized())
        return -1;

    result = fb_sfxAudioPlay(filename);
    if (result != 0)
        return -1;

    fb_sfxRuntimeLock();
    g_audio_loop = 1;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Stream feeder                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioFeed()

    Feed decoded audio data to the mixer.

    This function is called periodically by the streaming
    subsystem. For now it simply reads raw data from the file.

    A real implementation would decode formats such as:

        • WAV
        • OGG
        • FLAC
        • MP3
*/

int fb_sfxAudioFeed(float *buffer, int frames)
{
    int produced = 0;
    int out_channels;

    if (!buffer || frames <= 0)
        return 0;

    fb_sfxRuntimeLock();
    if (!g_audio_playing ||
        !g_audio_data ||
        g_audio_paused ||
        g_audio_frames <= 0 ||
        g_audio_channels <= 0)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    out_channels = __fb_sfx ? __fb_sfx->output_channels : FB_SFX_DEFAULT_CHANNELS;

    while (produced < frames)
    {
        float left;
        float right;

        if (g_audio_sample_step <= 0.0f)
            g_audio_sample_step = 1.0f;

        while (g_audio_sample_pos >= (float)g_audio_frames)
        {
            if (g_audio_loop)
            {
                g_audio_sample_pos -= (float)g_audio_frames;
            }
            else
            {
                fb_sfxAudioStopInternal();
                fb_sfxRuntimeUnlock();
                return produced;
            }
        }

        fb_sfxAudioReadFrame(g_audio_sample_pos, &left, &right);

        if (out_channels == 1)
        {
            buffer[produced] = (left + right) * 0.5f;
        }
        else
        {
            buffer[(produced * out_channels)] = left;
            buffer[(produced * out_channels) + 1] = right;
        }

        g_audio_sample_pos += g_audio_sample_step;
        if (g_audio_sample_pos >= (float)g_audio_frames)
            g_audio_position = g_audio_frames;
        else
            g_audio_position = (int)g_audio_sample_pos;
        produced++;
    }

    fb_sfxRuntimeUnlock();
    return produced;
}


/* end of sfx_audio_play.c */
