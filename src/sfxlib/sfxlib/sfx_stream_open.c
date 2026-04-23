/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_open.c

    Purpose:

        Implement the STREAM OPEN command.

        This command opens an audio stream source that will be
        incrementally decoded and fed into the sfxlib mixer.

        Unlike AUDIO PLAY, which is intended for simple playback,
        the STREAM interface exposes low-level control over the
        playback position and streaming lifecycle.

    Responsibilities:

        • open an audio stream file
        • initialize streaming state
        • prepare the decoder pipeline
        • expose the stream to the mixer subsystem

    This file intentionally does NOT contain:

        • audio decoding implementations
        • mixer algorithms
        • platform driver interaction

    Architectural overview:

        STREAM OPEN "file"
              │
        streaming subsystem
              │
        decoder pipeline
              │
        mixer input stream
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Stream state                                                              */
/* ------------------------------------------------------------------------- */

/*
    Only a single active stream is supported in this initial design.

    This keeps the architecture simple while the streaming subsystem
    is being developed.
*/

FILE *g_stream_file = NULL;
float *g_stream_data = NULL;
int   g_stream_frames = 0;
int   g_stream_channels = 0;
int   g_stream_samplerate = 0;
int   g_stream_open = 0;
int   g_stream_position = 0;


/* ------------------------------------------------------------------------- */
/* Internal helper                                                           */
/* ------------------------------------------------------------------------- */

static void fb_sfxStreamCloseInternal(void)
{
    if (g_stream_file)
    {
        fclose(g_stream_file);
        g_stream_file = NULL;
    }

    if (g_stream_data)
    {
        free(g_stream_data);
        g_stream_data = NULL;
    }

    g_stream_frames = 0;
    g_stream_channels = 0;
    g_stream_samplerate = 0;
    g_stream_open = 0;
    g_stream_position = 0;
}


/* ------------------------------------------------------------------------- */
/* STREAM OPEN                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamOpen()

    Open an audio file for streaming playback.
*/

int fb_sfxStreamOpen(const char *filename)
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
        Close any previous stream
    */

    fb_sfxStreamCloseInternal();

    if (fb_sfxDecodeFile(filename,
                         &decoded,
                         &frames,
                         &channels,
                         &sample_rate) != 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    g_stream_data = decoded;
    g_stream_frames = frames;
    g_stream_channels = channels;
    g_stream_samplerate = sample_rate;
    g_stream_open = 1;
    g_stream_position = 0;

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* STREAM STATUS                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamIsOpen()

    Return non-zero if a stream is currently open.
*/

int fb_sfxStreamIsOpen(void)
{
    int is_open;

    fb_sfxRuntimeLock();
    is_open = g_stream_open;
    fb_sfxRuntimeUnlock();

    return is_open;
}


/* ------------------------------------------------------------------------- */
/* STREAM FILE HANDLE                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamFile()

    Return the internal FILE pointer used by the stream subsystem.
*/

FILE *fb_sfxStreamFile(void)
{
    return g_stream_file;
}


/* end of sfx_stream_open.c */
