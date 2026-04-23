/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_play.c

    Purpose:

        Implement the STREAM PLAY command.

        This command begins playback of an already-opened stream
        created by STREAM OPEN.

        Unlike AUDIO PLAY, which hides the streaming subsystem,
        STREAM PLAY exposes explicit control of the playback state.

    Responsibilities:

        • start playback of an opened stream
        • reset playback state when necessary
        • expose stream state to the mixer

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    Stream file state is owned by sfx_stream_open.c
*/

extern FILE *g_stream_file;
extern float *g_stream_data;
extern int   g_stream_frames;
extern int   g_stream_channels;
extern int   g_stream_open;
extern int   g_stream_position;


/* ------------------------------------------------------------------------- */
/* Playback state                                                            */
/* ------------------------------------------------------------------------- */

int g_stream_playing = 0;
int g_stream_paused  = 0;


/* ------------------------------------------------------------------------- */
/* STREAM PLAY                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamPlay()

    Begin playback of the currently opened stream.
*/

int fb_sfxStreamPlay(void)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (!g_stream_open || !g_stream_data)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_stream_play: no stream open");
        return -1;
    }

    g_stream_playing = 1;
    g_stream_paused  = 0;

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* STREAM PLAYBACK STATE                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamPlaying()

    Return non-zero if a stream is actively playing.
*/

int fb_sfxStreamPlaying(void)
{
    int playing;

    fb_sfxRuntimeLock();
    playing = g_stream_playing && !g_stream_paused;
    fb_sfxRuntimeUnlock();

    return playing;
}


/*
    fb_sfxStreamPaused()

    Return non-zero if a stream is currently paused.
*/

int fb_sfxStreamPaused(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = g_stream_paused;
    fb_sfxRuntimeUnlock();

    return paused;
}


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamStopInternal()

    Stop stream playback without closing the stream file.
*/

void fb_sfxStreamStopInternal(void)
{
    fb_sfxRuntimeLock();
    g_stream_playing = 0;
    g_stream_paused  = 0;
    fb_sfxRuntimeUnlock();
}

int fb_sfxStreamFeed(float *buffer, int frames)
{
    int produced = 0;
    int out_channels;

    if (!buffer || frames <= 0)
        return 0;

    fb_sfxRuntimeLock();
    if (!g_stream_playing || g_stream_paused || !g_stream_data)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    out_channels = __fb_sfx ? __fb_sfx->output_channels : FB_SFX_DEFAULT_CHANNELS;

    while (produced < frames && g_stream_position < g_stream_frames)
    {
        const float *src = g_stream_data + (g_stream_position * g_stream_channels);
        float left = src[0];
        float right = (g_stream_channels > 1) ? src[1] : src[0];

        if (out_channels == 1)
            buffer[produced] = (left + right) * 0.5f;
        else
        {
            buffer[(produced * out_channels)] = left;
            buffer[(produced * out_channels) + 1] = right;
        }

        g_stream_position++;
        produced++;
    }

    if (g_stream_position >= g_stream_frames)
        fb_sfxStreamStopInternal();

    fb_sfxRuntimeUnlock();
    return produced;
}


/* end of sfx_stream_play.c */
