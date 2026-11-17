/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: haiku_audio_stream.cpp

    Purpose:

        Provide the Haiku Media Kit streaming interface used by
        the sfxlib audio backend.

        This module connects the sfxlib software mixer to the
        Haiku audio subsystem by using the BSoundPlayer callback
        mechanism.

    Responsibilities:

        • create and manage a BSoundPlayer instance
        • provide a real-time audio callback
        • request mixed audio frames from the sfxlib mixer
        • feed those frames into the Media Kit output stream

    This file intentionally does NOT contain:

        • synthesis logic
        • command processing
        • mixer algorithms
        • driver selection logic

    Architectural overview:

        BASIC program
              │
        sfxlib commands
              │
        software mixer
              │
        Haiku audio stream callback  ← this file
              │
        Haiku Media Kit (BSoundPlayer)
              │
        audio hardware
*/

#ifndef DISABLE_HAIKU

#include "fb_sfx_haiku.h"

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <SoundPlayer.h>
#include <MediaDefs.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Global stream objects                                                     */
/* ------------------------------------------------------------------------- */

static BSoundPlayer *g_player = NULL;


/* ------------------------------------------------------------------------- */
/* Debug helpers                                                             */
/* ------------------------------------------------------------------------- */

static int g_stream_debug_initialized = 0;
static int g_stream_debug_enabled = 0;

static void fb_hHaikuStreamInitDebug(void)
{
    const char *env;

    if (g_stream_debug_initialized)
        return;

    g_stream_debug_initialized = 1;

    env = getenv("HAIKU_SFX_STREAM_DEBUG");
    g_stream_debug_enabled = (env && *env && *env != '0');
}

static int fb_hHaikuStreamDebugEnabled(void)
{
    fb_hHaikuStreamInitDebug();
    return g_stream_debug_enabled;
}

#define HAIKU_STREAM_DBG(...) \
    do { if (fb_hHaikuStreamDebugEnabled()) \
        fprintf(stderr,"HAIKU_STREAM: " __VA_ARGS__); } while (0)


/* ------------------------------------------------------------------------- */
/* Audio callback                                                            */
/* ------------------------------------------------------------------------- */

/*
    audio_callback()

    Called by the Haiku Media Kit whenever the output device
    requires more audio frames.

    The callback requests fresh samples from the sfxlib mixer.
*/

static void audio_callback(
    void *cookie,
    void *buffer,
    size_t size,
    const media_raw_audio_format &format
)
{
    (void)cookie;

    float *out;
    size_t channel_count;
    size_t frame_size;
    size_t frame_count;

    if ((buffer == NULL) || (size == 0u))
        return;

    if (!fb_sfx_context)
    {
        memset(buffer, 0, size);
        return;
    }

    channel_count = (size_t)format.channel_count;
    if ((channel_count == 0u) ||
        (channel_count > ((size_t)-1 / sizeof(float))))
    {
        memset(buffer, 0, size);
        return;
    }

    frame_size = channel_count * sizeof(float);
    if ((size % frame_size) != 0u)
    {
        memset(buffer, 0, size);
        return;
    }

    frame_count = size / frame_size;
    if (frame_count > (size_t)INT_MAX)
    {
        memset(buffer, 0, size);
        return;
    }

    out = (float*)buffer;

    /*
        Request new samples from the mixer.

        The mixer writes directly into the provided output buffer.
    */

    /*
        This helper path mixes the current voice state directly into
        the callback buffer.

        The broader sfxlib runtime normally feeds drivers via
        fb_sfxUpdate(), but this stream module is still useful as a
        small standalone Media Kit bridge and should at least stay
        buildable against the shared mixer API.
    */
    fb_sfxMixFrame(out, (int)frame_count);
}


/* ------------------------------------------------------------------------- */
/* Stream initialization                                                     */
/* ------------------------------------------------------------------------- */

int fb_hHaikuAudioStreamInit(void)
{
    media_raw_audio_format format;
    size_t buffer_frames;
    size_t channel_count;

    fb_hHaikuStreamInitDebug();

    if (g_player)
        return 0;

    if ((fb_sfx_haiku.sample_rate <= 0) ||
        (fb_sfx_haiku.channels <= 0) ||
        (fb_sfx_haiku.buffer_frames <= 0))
        return -1;

    buffer_frames = (size_t)fb_sfx_haiku.buffer_frames;
    channel_count = (size_t)fb_sfx_haiku.channels;
    if (buffer_frames > ((size_t)-1 / sizeof(float)) / channel_count)
        return -1;

    HAIKU_STREAM_DBG("initializing audio stream\n");

    memset(&format, 0, sizeof(format));

    format.frame_rate = (float)fb_sfx_haiku.sample_rate;
    format.channel_count = (uint32)channel_count;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_HOST_ENDIAN;
    format.buffer_size = buffer_frames * sizeof(float) * channel_count;

    g_player = new BSoundPlayer(
        &format,
        "FreeBASIC sfxlib",
        audio_callback
    );

    if (!g_player)
    {
        HAIKU_STREAM_DBG("failed to create BSoundPlayer\n");
        return -1;
    }

    if (g_player->InitCheck() != B_OK)
    {
        HAIKU_STREAM_DBG("BSoundPlayer initialization failed\n");
        delete g_player;
        g_player = NULL;
        return -1;
    }

    g_player->SetHasData(true);
    g_player->Start();

    fb_sfx_haiku.running = 1;

    HAIKU_STREAM_DBG(
        "stream started (rate=%d channels=%d)\n",
        fb_sfx_haiku.sample_rate,
        fb_sfx_haiku.channels
    );

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Stream shutdown                                                           */
/* ------------------------------------------------------------------------- */

void fb_hHaikuAudioStreamShutdown(void)
{
    if (!g_player)
        return;

    HAIKU_STREAM_DBG("stopping audio stream\n");

    g_player->Stop();

    delete g_player;
    g_player = NULL;

    fb_sfx_haiku.running = 0;
}


/* ------------------------------------------------------------------------- */
/* Stream status                                                             */
/* ------------------------------------------------------------------------- */

int fb_hHaikuAudioStreamRunning(void)
{
    return (g_player != NULL);
}

#endif

/* end of haiku_audio_stream.cpp */
