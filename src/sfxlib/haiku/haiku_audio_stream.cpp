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

    float *out = (float*)buffer;

    int frames;
    int samples;

    if (!__fb_sfx)
    {
        memset(buffer, 0, size);
        return;
    }

    samples = size / sizeof(float);
    frames = samples / format.channel_count;

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
    fb_sfxMixFrame(out, frames);
}


/* ------------------------------------------------------------------------- */
/* Stream initialization                                                     */
/* ------------------------------------------------------------------------- */

int fb_hHaikuAudioStreamInit(void)
{
    media_raw_audio_format format;

    fb_hHaikuStreamInitDebug();

    if (g_player)
        return 0;

    HAIKU_STREAM_DBG("initializing audio stream\n");

    memset(&format, 0, sizeof(format));

    format.frame_rate = fb_sfx_haiku.sample_rate;
    format.channel_count = fb_sfx_haiku.channels;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_HOST_ENDIAN;
    format.buffer_size = fb_sfx_haiku.buffer_frames *
                         sizeof(float) *
                         fb_sfx_haiku.channels;

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
