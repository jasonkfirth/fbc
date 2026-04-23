/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_haiku.cpp

    Purpose:

        Implement Haiku audio capture support for sfxlib.

        This module provides the capture-side backend glue for
        the Haiku platform. In its current form it establishes
        the capture lifecycle, buffer management, and a safe
        internal data path that higher layers can exercise
        immediately.

    Responsibilities:

        • manage Haiku capture lifecycle
        • maintain a capture ring buffer
        • expose capture start/stop/read entry points
        • provide a safe default silence-producing backend
        • prepare the architecture for later Media Kit input integration

    This file intentionally does NOT contain:

        • mixer logic
        • playback driver logic
        • synthesis logic
        • file encoding for captured audio
        • a full Media Kit producer/consumer graph

    Architectural note:

        The immediate goal is correctness, testability, and a stable
        contract with the rest of sfxlib.

        This implementation therefore behaves conservatively:
        it supports initialization, start/stop semantics, and reading
        captured frames through a ring buffer, while defaulting to
        silence when no real capture source has been attached yet.

        A later implementation can replace the silence source with
        real Media Kit capture without changing the public/backend API.
*/

#ifndef DISABLE_HAIKU

#include "fb_sfx_haiku.h"

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Internal capture state                                                    */
/* ------------------------------------------------------------------------- */

/*
    Capture ring buffer

    The buffer stores interleaved float PCM samples. The write side is
    currently fed either by an internal silence generator or, in a future
    revision, by a real Haiku Media Kit capture callback/thread.

    The read side is used by the generic sfxlib capture API.
*/

static float *g_capture_ring = NULL;
static int    g_capture_ring_frames = 0;
static int    g_capture_ring_channels = 0;

static int    g_capture_read_pos = 0;
static int    g_capture_write_pos = 0;
static int    g_capture_used_frames = 0;

static int    g_capture_initialized = 0;
static int    g_capture_running = 0;


/* ------------------------------------------------------------------------- */
/* Debug support                                                             */
/* ------------------------------------------------------------------------- */

static int g_capture_debug_initialized = 0;
static int g_capture_debug_enabled = 0;


static void fb_sfxHaikuCaptureInitDebug(void)
{
    const char *env;

    if (g_capture_debug_initialized)
        return;

    g_capture_debug_initialized = 1;

    env = getenv("HAIKU_SFXLIB_CAPTURE_DEBUG");
    g_capture_debug_enabled = (env && *env && *env != '0');
}


static int fb_sfxHaikuCaptureDebugEnabled(void)
{
    fb_sfxHaikuCaptureInitDebug();
    return g_capture_debug_enabled;
}


#define HAIKU_CAPTURE_DEBUG(...) \
    do { \
        if (fb_sfxHaikuCaptureDebugEnabled()) \
            fprintf(stderr, "HAIKU_SFX_CAPTURE: " __VA_ARGS__); \
    } while (0)


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

/*
    Return the number of samples contained in a frame block.

    This helper exists to keep all size calculations explicit and to avoid
    repeating frame/channel multiplication throughout the file.
*/

static size_t fb_sfxHaikuCaptureSampleCount(int frames, int channels)
{
    if (frames <= 0 || channels <= 0)
        return 0;

    return (size_t)frames * (size_t)channels;
}


/*
    Ensure the capture ring exists.

    The ring capacity is expressed in frames, not samples. Internally the
    allocation size is capacity × channels × sizeof(float).
*/

static int fb_sfxHaikuCaptureEnsureRing(int frames, int channels)
{
    float *new_buffer;
    size_t sample_count;
    size_t byte_count;

    if (frames <= 0 || channels <= 0)
        return -1;

    if (g_capture_ring &&
        g_capture_ring_frames == frames &&
        g_capture_ring_channels == channels)
    {
        return 0;
    }

    sample_count = fb_sfxHaikuCaptureSampleCount(frames, channels);
    if (sample_count == 0)
        return -1;

    byte_count = sample_count * sizeof(float);

    new_buffer = (float*)realloc(g_capture_ring, byte_count);
    if (!new_buffer)
        return -1;

    g_capture_ring = new_buffer;
    g_capture_ring_frames = frames;
    g_capture_ring_channels = channels;

    memset(g_capture_ring, 0, byte_count);

    g_capture_read_pos = 0;
    g_capture_write_pos = 0;
    g_capture_used_frames = 0;

    HAIKU_CAPTURE_DEBUG(
        "ring allocated (%d frames, %d channels)\n",
        g_capture_ring_frames,
        g_capture_ring_channels
    );

    return 0;
}


/*
    Write one frame into the ring buffer.

    If the ring is full, the oldest frame is discarded. This policy is chosen
    deliberately because real-time capture should prefer the most recent data
    over preserving stale data indefinitely.
*/

static void fb_sfxHaikuCaptureWriteFrame(const float *frame)
{
    float *dst;
    size_t frame_samples;
    size_t frame_bytes;

    if (!g_capture_ring || !frame)
        return;

    if (g_capture_ring_frames <= 0 || g_capture_ring_channels <= 0)
        return;

    /*
        If full, drop the oldest frame first.
    */
    if (g_capture_used_frames >= g_capture_ring_frames)
    {
        g_capture_read_pos++;
        if (g_capture_read_pos >= g_capture_ring_frames)
            g_capture_read_pos = 0;

        g_capture_used_frames = g_capture_ring_frames - 1;
    }

    frame_samples = fb_sfxHaikuCaptureSampleCount(1, g_capture_ring_channels);
    frame_bytes = frame_samples * sizeof(float);

    dst = g_capture_ring + ((size_t)g_capture_write_pos * (size_t)g_capture_ring_channels);
    memcpy(dst, frame, frame_bytes);

    g_capture_write_pos++;
    if (g_capture_write_pos >= g_capture_ring_frames)
        g_capture_write_pos = 0;

    g_capture_used_frames++;
}


/*
    Read one frame from the ring buffer.

    Returns non-zero on success. If no captured data is available, the output
    frame is filled with silence and zero is returned.
*/

static int fb_sfxHaikuCaptureReadFrame(float *frame)
{
    const float *src;
    size_t frame_samples;
    size_t frame_bytes;

    if (!frame || g_capture_ring_channels <= 0)
        return 0;

    frame_samples = fb_sfxHaikuCaptureSampleCount(1, g_capture_ring_channels);
    frame_bytes = frame_samples * sizeof(float);

    if (!g_capture_ring || g_capture_used_frames <= 0)
    {
        memset(frame, 0, frame_bytes);
        return 0;
    }

    src = g_capture_ring + ((size_t)g_capture_read_pos * (size_t)g_capture_ring_channels);
    memcpy(frame, src, frame_bytes);

    g_capture_read_pos++;
    if (g_capture_read_pos >= g_capture_ring_frames)
        g_capture_read_pos = 0;

    g_capture_used_frames--;

    return 1;
}


/*
    Feed synthetic silence into the capture ring.

    This is the current default capture source. It keeps the entire capture
    pipeline active and testable even before real Media Kit capture is wired in.
*/

static void fb_sfxHaikuCaptureFeedSilence(int frames)
{
    float silent_frame[8];
    int i;
    int channels;

    channels = g_capture_ring_channels;

    if (channels <= 0)
        return;

    if (channels > (int)(sizeof(silent_frame) / sizeof(silent_frame[0])))
    {
        /*
            The current runtime design is expected to use small channel counts
            such as mono or stereo. If a larger count appears later, this can
            be replaced with a heap allocation without changing the interface.
        */
        return;
    }

    memset(silent_frame, 0, sizeof(silent_frame));

    for (i = 0; i < frames; i++)
        fb_sfxHaikuCaptureWriteFrame(silent_frame);
}


/* ------------------------------------------------------------------------- */
/* Capture lifecycle                                                         */
/* ------------------------------------------------------------------------- */

int fb_sfxHaikuCaptureInit(void)
{
    int ring_frames;
    int channels;

    fb_sfxHaikuCaptureInitDebug();

    if (g_capture_initialized)
        return 0;

    /*
        The capture subsystem depends on the general Haiku backend state.
        Ensure the platform layer is initialized first.
    */
    if (!fb_sfx_haiku.initialized)
    {
        if (fb_sfxHaikuInit() != 0)
            return -1;
    }

    channels = fb_sfx_haiku.channels;
    if (channels <= 0)
        channels = 2;

    /*
        Use a ring somewhat larger than the nominal device buffer so callers
        can poll in bursts without immediately losing all capture data.
    */
    ring_frames = fb_sfx_haiku.buffer_frames;
    if (ring_frames <= 0)
        ring_frames = 512;

    ring_frames *= 8;

    if (fb_sfxHaikuCaptureEnsureRing(ring_frames, channels) != 0)
        return -1;

    g_capture_initialized = 1;
    g_capture_running = 0;

    HAIKU_CAPTURE_DEBUG(
        "capture initialized (ring=%d frames channels=%d)\n",
        g_capture_ring_frames,
        g_capture_ring_channels
    );

    return 0;
}


int fb_sfxHaikuCaptureStart(void)
{
    if (!g_capture_initialized)
    {
        if (fb_sfxHaikuCaptureInit() != 0)
            return -1;
    }

    if (g_capture_running)
        return 0;

    g_capture_running = 1;

    HAIKU_CAPTURE_DEBUG("capture started\n");

    return 0;
}


void fb_sfxHaikuCaptureStop(void)
{
    if (!g_capture_initialized)
        return;

    g_capture_running = 0;

    HAIKU_CAPTURE_DEBUG("capture stopped\n");
}


/*
    Optional explicit shutdown helper.

    The public header does not currently expose this separately because the
    main backend shutdown can reclaim all resources. Keeping the helper local
    here makes teardown rules obvious and keeps the code bullet-proof.
*/

static void fb_sfxHaikuCaptureShutdown(void)
{
    g_capture_running = 0;
    g_capture_initialized = 0;

    if (g_capture_ring)
    {
        free(g_capture_ring);
        g_capture_ring = NULL;
    }

    g_capture_ring_frames = 0;
    g_capture_ring_channels = 0;
    g_capture_read_pos = 0;
    g_capture_write_pos = 0;
    g_capture_used_frames = 0;

    HAIKU_CAPTURE_DEBUG("capture shutdown\n");
}


/* ------------------------------------------------------------------------- */
/* Capture read path                                                         */
/* ------------------------------------------------------------------------- */

int fb_sfxHaikuCaptureRead(float *buffer, int frames)
{
    float frame_buffer[8];
    size_t frame_samples;
    size_t frame_bytes;
    int i;
    int channels;

    if (!buffer || frames <= 0)
        return 0;

    if (!g_capture_initialized)
        return 0;

    channels = g_capture_ring_channels;
    if (channels <= 0)
        return 0;

    if (channels > (int)(sizeof(frame_buffer) / sizeof(frame_buffer[0])))
        return -1;

    /*
        In the current implementation, when capture is running we synthesize
        silence into the ring on demand. This keeps the API behavior stable
        and fully testable without needing real device input yet.
    */
    if (g_capture_running && g_capture_used_frames < frames)
        fb_sfxHaikuCaptureFeedSilence(frames - g_capture_used_frames);

    frame_samples = fb_sfxHaikuCaptureSampleCount(1, channels);
    frame_bytes = frame_samples * sizeof(float);

    for (i = 0; i < frames; i++)
    {
        fb_sfxHaikuCaptureReadFrame(frame_buffer);
        memcpy(buffer + ((size_t)i * (size_t)channels), frame_buffer, frame_bytes);
    }

    HAIKU_CAPTURE_DEBUG("read %d capture frames\n", frames);

    return frames;
}


/* ------------------------------------------------------------------------- */
/* Internal injection helper for future real capture                         */
/* ------------------------------------------------------------------------- */

/*
    Feed externally captured frames into the Haiku capture ring.

    This function is not part of the public platform header yet, but it gives
    the later Media Kit capture path a clean place to inject frames without
    rewriting the buffering logic in this file.
*/

extern "C" void fb_sfxHaikuCaptureInject(const float *buffer, int frames)
{
    int i;
    int channels;
    const float *src;

    if (!buffer || frames <= 0)
        return;

    if (!g_capture_initialized)
        return;

    channels = g_capture_ring_channels;
    if (channels <= 0)
        return;

    src = buffer;

    for (i = 0; i < frames; i++)
    {
        fb_sfxHaikuCaptureWriteFrame(src);
        src += channels;
    }

    HAIKU_CAPTURE_DEBUG("injected %d capture frames\n", frames);
}


/* ------------------------------------------------------------------------- */
/* Backend teardown hook                                                     */
/* ------------------------------------------------------------------------- */

/*
    This symbol allows the main Haiku backend to reclaim capture resources
    cleanly if it chooses to call into the capture side during shutdown.

    Keeping the hook in this translation unit preserves ownership of the
    capture state and avoids hidden cross-file global cleanup.
*/

extern "C" void fb_sfxHaikuCaptureBackendShutdown(void)
{
    fb_sfxHaikuCaptureShutdown();
}


/* end of sfx_capture_haiku.cpp */

#endif
