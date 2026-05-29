/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_core.c

    Purpose:

        Implement the core control logic for the FreeBASIC sound
        subsystem.

        This file coordinates initialization, shutdown, and the
        primary audio processing flow between the mixer and the
        platform driver.

    Responsibilities:

        • initialize the runtime sound subsystem
        • select and initialize a platform audio driver
        • coordinate mixer output and driver delivery
        • provide the central audio processing loop

    This file intentionally does NOT contain:

        • audio synthesis algorithms
        • mixer implementation
        • platform-specific driver code
        • BASIC command implementations

    Architectural overview:

        BASIC program
              │
              ▼
        command layer
              │
              ▼
        mixer subsystem
              │
              ▼
        runtime mix buffer
              │
              ▼
        platform driver

    Design note:

        The runtime owns all audio buffers. Drivers are only responsible
        for delivering the generated samples to the operating system.
*/

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__DJGPP__)
#include <dos.h>
#else
#include <time.h>
#endif

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_driver_diag.h"
#include "fb_sfx_mixer.h"
#include "fb_sfx_buffer.h"
#include "fb_sfx_capture.h"

static void fb_sfxInitCoreRollbackLocked(void);
static int fb_sfxOutputQueueInitLocked(void);
static void fb_sfxOutputQueueShutdownLocked(void);
static int fb_sfxOutputQueueFillLocked(int frames);
static int fb_sfxOutputQueueDrainLocked(int frames);
static int fb_sfxMixFeedScratchEnsureLocked(int frames, int channels);
static void fb_sfxMixFeedScratchShutdownLocked(void);
static void fb_sfxSleepMs(unsigned long milliseconds);
static int fb_sfxCurrentDriverBlocksLocked(void);
static int fb_sfxCurrentDriverBlocks(void);
#if defined(__DJGPP__)
static int fb_sfxCurrentDriverIsNull(void);
static int fb_sfxUpdateDelayFrames(int msecs, int return_after_update);
#endif
static int fb_sfxDriverNameEquals(const char *a, const char *b);
static int fb_sfxDriverTryByName(const char *name);
static int g_foreground_feed_count = 0;
static float *g_fb_sfx_mix_feed_scratch = NULL;
static int g_fb_sfx_mix_feed_capacity = 0;


/* ------------------------------------------------------------------------- */
/* Runtime output queue                                                      */
/* ------------------------------------------------------------------------- */

/*
    Output queue

    Worker-driven backends do not always wake up at exact audio-period
    boundaries.  A small runtime-owned queue lets the mixer stay ahead
    of playback so ordinary scheduler jitter does not immediately become
    an audible underrun.
*/

static FB_SFX_RINGBUFFER g_output_queue;
static int g_output_queue_initialized = 0;

/* ------------------------------------------------------------------------- */
/* Subsystem initialization                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxInitCore()

    Initialize the sound subsystem.

    This function performs the following steps:

        1. Allocate runtime context
        2. Initialize mixer and buffers
        3. Attempt driver initialization
*/

int fb_sfxInitCore(void)
{
    int result;

    fb_sfxRuntimeLock();

    if (__fb_sfx == NULL)
    {
        SFX_DEBUG("sfx_core: runtime context missing");
        fb_sfxRuntimeUnlock();
        return -1;
    }

    if (__fb_sfx->initialized)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    __fb_sfx->shutting_down = 0;

    SFX_DEBUG("sfx_core: initializing sound subsystem");

    /* initialize shared runtime state */
    fb_sfxChannelInit();
    fb_sfxEnvelopeInit();

    /* initialize mixer */
    fb_sfxMixerInit();

    /* initialize buffers */
    fb_sfxMixBufferInit();
    fb_sfxCaptureBufferInit();

    if (!__fb_sfx->mixbuffer)
    {
        SFX_DEBUG("sfx_core: failed to allocate mix buffer");
        fb_sfxInitCoreRollbackLocked();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    if (fb_sfxOutputQueueInitLocked() != 0)
    {
        SFX_DEBUG("sfx_core: failed to allocate output queue");
        fb_sfxInitCoreRollbackLocked();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    /* initialize capture subsystem */
    fb_sfxCaptureInit();

    /* initialize platform driver */
    result = fb_sfxDriverInit();

    if (result != 0)
    {
        SFX_DEBUG("sfx_core: no audio driver available");
        fb_sfxInitCoreRollbackLocked();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    __fb_sfx->initialized = 1;

    SFX_DEBUG("sfx_core: initialization complete");

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Subsystem shutdown                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxExitCore()

    Shutdown the sound subsystem and release resources.
*/

void fb_sfxExitCore(void)
{
    fb_sfxRuntimeLock();

    if (__fb_sfx == NULL)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (!__fb_sfx->initialized)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    SFX_DEBUG("sfx_core: shutting down sound subsystem");
    __fb_sfx->shutting_down = 1;
    fb_sfxRuntimeUnlock();

    /*
        MIDI playback may be running on a worker thread.

        Stop and join it before tearing down the driver layer so the
        worker cannot continue dispatching events into a closed backend.
    */

    fb_sfxMidiStop();

    fb_sfxPlatformExit();

    fb_sfxDriverShutdown();

    fb_sfxRuntimeLock();

    fb_sfxCaptureShutdown();

    fb_sfxMixBufferShutdown();
    fb_sfxOutputQueueShutdownLocked();
    fb_sfxCaptureBufferShutdown();
    fb_sfxMixFeedScratchShutdownLocked();

    fb_sfxMixerShutdown();

    __fb_sfx->initialized = 0;
    fb_sfxRuntimeUnlock();
}

static int fb_sfxDriverFallback(const SFXDRIVER *failed_driver);
static void fb_sfxMixFeedSource(int frames, int (*feed_fn)(float *buffer, int frames));

static void fb_sfxInitCoreRollbackLocked(void)
{
    if (!__fb_sfx)
        return;

    __fb_sfx->driver = NULL;
    __fb_sfx->initialized = 0;

    fb_sfxCaptureShutdown();
    fb_sfxMixBufferShutdown();
    fb_sfxOutputQueueShutdownLocked();
    fb_sfxCaptureBufferShutdown();
    fb_sfxMixFeedScratchShutdownLocked();
    fb_sfxMixerShutdown();
}

static int fb_sfxMixFeedScratchEnsureLocked(int frames, int channels)
{
    int required_samples;
    int next_capacity;
    float *next_scratch;

    if (frames <= 0 || channels <= 0)
        return -1;

    required_samples = frames * channels;
    if (required_samples <= g_fb_sfx_mix_feed_capacity)
        return 0;

    next_capacity = g_fb_sfx_mix_feed_capacity > 0
        ? g_fb_sfx_mix_feed_capacity
        : 1024;

    while (next_capacity < required_samples)
        next_capacity <<= 1;

    next_scratch = (float *)realloc(g_fb_sfx_mix_feed_scratch,
                                   (size_t)next_capacity * sizeof(float));
    if (!next_scratch)
        return -1;

    g_fb_sfx_mix_feed_scratch = next_scratch;
    g_fb_sfx_mix_feed_capacity = next_capacity;

    return 0;
}

static void fb_sfxMixFeedScratchShutdownLocked(void)
{
    if (g_fb_sfx_mix_feed_scratch)
    {
        free(g_fb_sfx_mix_feed_scratch);
        g_fb_sfx_mix_feed_scratch = NULL;
        g_fb_sfx_mix_feed_capacity = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Audio processing                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxUpdate()

    Generate audio frames and deliver them to the platform driver.

    This function is typically called periodically to maintain
    continuous audio playback.
*/

void fb_sfxUpdate(int frames)
{
    int chunk;
    int chunk_limit;
    int remaining;
    int written;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();

    if (frames <= 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (__fb_sfx->mixbuffer == NULL)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    chunk_limit = (__fb_sfx->buffer_frames > 0)
        ? __fb_sfx->buffer_frames
        : FB_SFX_DEFAULT_BUFFER;

    if (chunk_limit <= 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    /*
        The mix buffer is sized for one device-sized block.  A caller may ask
        for a larger update, especially in tests and foreground commands, so
        large updates are delivered as a series of bounded blocks.
    */

    remaining = frames;

    while (remaining > 0)
    {
        chunk = (remaining > chunk_limit) ? chunk_limit : remaining;

        if (fb_sfxOutputQueueFillLocked(chunk) != 0)
            break;

        written = fb_sfxOutputQueueDrainLocked(chunk);
        if (written <= 0)
        {
            /*
                If playback cannot make progress, keep the worker from
                spinning at full speed by yielding briefly.
            */
            fb_sfxRuntimeUnlock();
            fb_sfxSleepMs(1);
            fb_sfxRuntimeLock();
            break;
        }

        remaining -= written;

        if (written < chunk)
            break;
    }

    fb_sfxRuntimeUnlock();
}

/* ------------------------------------------------------------------------- */
/* Foreground audio delivery                                                 */
/* ------------------------------------------------------------------------- */

/*
    Some classic BASIC sound statements are foreground operations from the
    programmer's point of view.  They must not return to a short demo program
    before the requested tone has reached the platform driver.

    Background drivers pause their worker feed while this helper is active, so
    the command thread can write the exact number of frames it is responsible
    for without racing the driver thread.
*/

void fb_sfxRunForeground(int frames)
{
    int driver_blocks_audio;
    int step;
    int tick_frames;
    int samplerate;

    if (frames <= 0)
        return;

    samplerate = (__fb_sfx && __fb_sfx->samplerate > 0)
        ? __fb_sfx->samplerate
        : 0;

    tick_frames = (samplerate > 0)
        ? (samplerate / 20)
        : 220;

    driver_blocks_audio = fb_sfxCurrentDriverBlocks();

    /*
        DOS Sound Blaster and PC speaker writes consume wall-clock time while
        the caller is inside driver->write().  Sleeping again after those
        writes creates audible gaps between chunks.  Worker-driven and null
        drivers still need the explicit delay to preserve foreground command
        timing.
    */
    if (driver_blocks_audio && __fb_sfx && __fb_sfx->buffer_frames > 0)
        tick_frames = __fb_sfx->buffer_frames;

    if (tick_frames <= 0)
        tick_frames = 220;

    fb_sfxForegroundFeedBegin();

    fb_sfxRuntimeLock();
    if (g_output_queue_initialized)
        fb_sfxRingBufferClear(&g_output_queue);
    fb_sfxRuntimeUnlock();

    while (frames > 0)
    {
        unsigned long milliseconds;

        step = (frames > tick_frames) ? tick_frames : frames;

        fb_sfxUpdate(step);

        if (samplerate > 0 && !driver_blocks_audio)
        {
            milliseconds = (unsigned long)(((unsigned long long)step * 1000ULL) / (unsigned long long)samplerate);
            if (milliseconds == 0)
                milliseconds = 1;
            fb_sfxSleepMs(milliseconds);
        }

        frames -= step;
    }

    fb_sfxForegroundFeedEnd();
}

/*
    DOS cooperative delay

    The DOS drivers are synchronous and do not own a worker thread.  Background
    sound commands still need time to advance while a program is waiting in
    SLEEP/DELAY.  fb_Delay() calls this hook before entering the normal DOS
    sleep path, allowing sfxlib to turn that wait time into generated audio.

    Drivers that block in write() consume the wait themselves and return 1 so
    fb_Delay() does not sleep a second time.  The null driver is pumped for
    diagnostics but returns 0 so the caller still observes the requested delay.
*/

int fb_sfxCooperativeDelay(int msecs)
{
#if defined(__DJGPP__)
    if (msecs <= 0)
        return 0;

    if (fb_sfxCurrentDriverBlocks())
        return fb_sfxUpdateDelayFrames(msecs, 1);

    if (fb_sfxCurrentDriverIsNull())
        (void)fb_sfxUpdateDelayFrames(msecs, 0);

    return 0;
#else
    (void)msecs;
    return 0;
#endif
}

static void fb_sfxSleepMs(unsigned long milliseconds)
{
    if (milliseconds == 0)
        return;

#if defined(_WIN32)
    Sleep((DWORD)milliseconds);
#elif defined(__DJGPP__)
    delay((unsigned)milliseconds);
#else
    {
        struct timespec req;

        req.tv_sec = (time_t)(milliseconds / 1000UL);
        req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
        nanosleep(&req, NULL);
    }
#endif
}

static int fb_sfxCurrentDriverBlocksLocked(void)
{
    const SFXDRIVER *driver;

    if (!__fb_sfx || __fb_sfx->shutting_down)
        return 0;

    driver = __fb_sfx->driver;
    if (!driver)
        return 0;

    return ((driver->capabilities & FB_SFX_DRIVER_CAP_BLOCKING) != 0);
}

static int fb_sfxCurrentDriverBlocks(void)
{
    int result;

    fb_sfxRuntimeLock();
    result = fb_sfxCurrentDriverBlocksLocked();
    fb_sfxRuntimeUnlock();

    return result;
}

#if defined(__DJGPP__)
static int fb_sfxCurrentDriverIsNull(void)
{
    int result;

    result = 0;

    fb_sfxRuntimeLock();

    if (__fb_sfx && __fb_sfx->driver && !__fb_sfx->shutting_down)
        result = fb_sfxDriverNameEquals(__fb_sfx->driver->name, "null");

    fb_sfxRuntimeUnlock();

    return result;
}

static int fb_sfxUpdateDelayFrames(int msecs, int return_after_update)
{
    long long frames_left;
    int buffer_frames;
    int samplerate;
    int step_frames;

    fb_sfxRuntimeLock();

    samplerate = (__fb_sfx && __fb_sfx->samplerate > 0)
        ? __fb_sfx->samplerate
        : 0;

    buffer_frames = (__fb_sfx && __fb_sfx->buffer_frames > 0)
        ? __fb_sfx->buffer_frames
        : 0;

    fb_sfxRuntimeUnlock();

    if (samplerate <= 0)
        return 0;

    frames_left = (((long long)msecs * (long long)samplerate) + 999LL) / 1000LL;
    if (frames_left <= 0)
        frames_left = 1;

    step_frames = (buffer_frames > 0)
        ? buffer_frames
        : (samplerate / 20);

    if (step_frames <= 0)
        step_frames = 220;

    while (frames_left > 0)
    {
        int step;

        step = (frames_left > (long long)step_frames)
            ? step_frames
            : (int)frames_left;

        fb_sfxUpdate(step);
        frames_left -= step;
    }

    return return_after_update;
}
#endif


/* ------------------------------------------------------------------------- */
/* Driver selection                                                          */
/* ------------------------------------------------------------------------- */

static int fb_sfxDriverNameEquals(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    while (*a && *b)
    {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z')
            ca = (char)(ca - 'A' + 'a');

        if (cb >= 'A' && cb <= 'Z')
            cb = (char)(cb - 'A' + 'a');

        if (ca != cb)
            return 0;

        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0');
}

static int fb_sfxDriverIndexOf(const SFXDRIVER *driver)
{
    int i;

    if (!driver)
        return -1;

    for (i = 0; __fb_sfx_drivers_list[i]; ++i)
    {
        if (__fb_sfx_drivers_list[i] == driver)
            return i;
    }

    return -1;
}

static void fb_sfxMixFeedSource(int frames, int (*feed_fn)(float *buffer, int frames))
{
    float *scratch;
    int channels;
    int samples;
    int produced;
    int i;

    if (!__fb_sfx || !__fb_sfx->mixbuffer || !feed_fn || frames <= 0)
        return;

    channels = (__fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    if (fb_sfxMixFeedScratchEnsureLocked(frames, channels) != 0)
        return;

    samples = frames * channels;
    scratch = g_fb_sfx_mix_feed_scratch;
    memset(scratch, 0, (size_t)samples * sizeof(float));
    produced = feed_fn(scratch, frames);
    if (produced > frames)
        produced = frames;

    for (i = 0; i < produced * channels; ++i)
        __fb_sfx->mixbuffer[i] = fb_sfxClampSample(__fb_sfx->mixbuffer[i] + scratch[i]);
}

static int fb_sfxOutputQueueInitLocked(void)
{
    int queue_frames;

    if (!__fb_sfx)
        return -1;

    if (g_output_queue_initialized)
        return 0;

    queue_frames = __fb_sfx->buffer_size;
    if (queue_frames <= 0)
        queue_frames = FB_SFX_DEFAULT_BUFFER;

    /*
        Two runtime-sized blocks provide enough slack for ordinary worker
        thread jitter without moving too far away from the requested
        device buffer size.
    */
    queue_frames *= 2;

    if (fb_sfxRingBufferInit(&g_output_queue,
                             queue_frames,
                             __fb_sfx->output_channels) != 0)
    {
        return -1;
    }

    g_output_queue_initialized = 1;
    return 0;
}

static void fb_sfxOutputQueueShutdownLocked(void)
{
    if (!g_output_queue_initialized)
        return;

    fb_sfxRingBufferShutdown(&g_output_queue);
    g_output_queue_initialized = 0;
}

static int fb_sfxOutputQueueFillLocked(int frames)
{
    int target_frames;
    int max_frames;

    if (!__fb_sfx || !__fb_sfx->mixbuffer)
        return -1;

    if (!g_output_queue_initialized && fb_sfxOutputQueueInitLocked() != 0)
        return -1;

    if (frames <= 0)
        frames = __fb_sfx->buffer_size;

    max_frames = (__fb_sfx->buffer_frames > 0)
        ? __fb_sfx->buffer_frames
        : frames;
    if (max_frames <= 0)
        max_frames = FB_SFX_DEFAULT_BUFFER;

    target_frames = frames;
    if (__fb_sfx->buffer_size > target_frames)
        target_frames = __fb_sfx->buffer_size;

    if (target_frames > g_output_queue.frames)
        target_frames = g_output_queue.frames;

    while (fb_sfxRingBufferAvailable(&g_output_queue) < target_frames)
    {
        int free_frames;
        int frames_this_pass;
        int written;

        free_frames = fb_sfxRingBufferFree(&g_output_queue);
        if (free_frames <= 0)
            break;

        frames_this_pass = free_frames;
        if (frames_this_pass > max_frames)
            frames_this_pass = max_frames;

        /* generate audio using the mixer */
        fb_sfxMixerProcess(frames_this_pass);

        /* mix decoded playback sources into the same live output buffer */
        fb_sfxMixFeedSource(frames_this_pass, fb_sfxAudioFeed);
        fb_sfxMixFeedSource(frames_this_pass, fb_sfxStreamFeed);

        fb_sfxMixerDiagnostics(__fb_sfx->mixbuffer, frames_this_pass);

        written = fb_sfxRingBufferWrite(&g_output_queue,
                                        __fb_sfx->mixbuffer,
                                        frames_this_pass);
        if (written != frames_this_pass)
            return -1;
    }

    return 0;
}

static int fb_sfxOutputQueueDrainLocked(int frames)
{
    const SFXDRIVER *driver;
    const char *last_driver_name;
    int channels;
    int queued;
    int drained;
    int written;
    int zero_retry_count;
    int zero_retry_limit;

    if (!__fb_sfx || !__fb_sfx->mixbuffer || frames <= 0)
        return 0;

    if (__fb_sfx->shutting_down)
        return 0;

    if (!g_output_queue_initialized)
        return 0;

    queued = fb_sfxRingBufferAvailable(&g_output_queue);
    if (queued <= 0)
        return 0;

    if (frames > queued)
        frames = queued;

    if (__fb_sfx->buffer_frames > 0 && frames > __fb_sfx->buffer_frames)
        frames = __fb_sfx->buffer_frames;

    drained = fb_sfxRingBufferRead(&g_output_queue, __fb_sfx->mixbuffer, frames);
    if (drained <= 0)
        return 0;

    channels = (__fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    driver = __fb_sfx->driver;
    if (driver && driver->name)
    {
        fb_sfxDriverStatsRecordQueueFill(driver->name, queued);
        fb_sfxDriverStatsRecordQueueFill(driver->name, queued - drained);
    }

    last_driver_name = driver ? driver->name : NULL;
    written = 0;
    zero_retry_count = 0;
    zero_retry_limit = 4;

    while (driver && driver->write)
    {
        int result;
        const float *write_buffer;
        int write_frames;
        int (*write_proc)(const float *samples, int frames);

        if (!__fb_sfx ||
            __fb_sfx->driver != driver ||
            __fb_sfx->shutting_down ||
            !__fb_sfx->mixbuffer ||
            !driver ||
            !driver->write)
        {
            driver = (__fb_sfx) ? __fb_sfx->driver : NULL;
            continue;
        }

        write_buffer = __fb_sfx->mixbuffer + (written * channels);
        write_frames = drained - written;
        last_driver_name = driver->name;

        /*
            Platform writes may block while the OS audio server applies
            backpressure.  Keep the runtime lock away from that call so
            command-layer code can continue to queue voices.

            The separate driver-I/O lock prevents foreground commands and
            driver feeder threads from entering the same platform audio API at
            the same time.  WASAPI and WinMM both treat overlapping writes as
            a real driver error, so the active driver is rechecked after the
            I/O lock is acquired.
        */
        fb_sfxRuntimeUnlock();
        fb_sfxDriverIoLock();
        fb_sfxRuntimeLock();

        if (!__fb_sfx ||
            __fb_sfx->driver != driver ||
            __fb_sfx->shutting_down ||
            !driver ||
            !driver->write)
        {
            fb_sfxDriverIoUnlock();
            driver = (__fb_sfx) ? __fb_sfx->driver : NULL;
            continue;
        }
        write_proc = driver->write;

        fb_sfxRuntimeUnlock();
        result = write_proc(write_buffer, write_frames);
        fb_sfxRuntimeLock();
        fb_sfxDriverIoUnlock();

        fb_sfxDriverStatsRecordWrite(driver->name, write_frames, result);

        if (result > 0)
        {
            if (result > write_frames)
            {
                SFX_DEBUG("sfx_core: driver '%s' over-reported write (%d > %d)",
                          driver->name ? driver->name : "(null)",
                          result,
                          write_frames);
                result = write_frames;
            }

            written += result;
            zero_retry_count = 0;

            if (written >= drained)
                return written;

            continue;
        }
        else if (result == 0)
        {
            if (zero_retry_count < zero_retry_limit)
            {
                ++zero_retry_count;

                /*
                    Treat transient zero-progress writes as backpressure and
                    retry after a short delay.  A zero return is
                    backpressure, not a fatal driver-lost signal.
                */
                fb_sfxRuntimeUnlock();
                fb_sfxSleepMs(1);
                fb_sfxRuntimeLock();

                continue;
            }

            SFX_DEBUG("sfx_core: driver '%s' accepted no frames after %d retries",
                      driver->name ? driver->name : "(null)",
                      zero_retry_count);

            break;
        }

        if (fb_sfxDriverFallback(driver) != 0)
            break;

        driver = (__fb_sfx) ? __fb_sfx->driver : NULL;
        if (driver)
            last_driver_name = driver->name;
    }

    if (written < drained && last_driver_name)
        fb_sfxDriverStatsRecordDrop(last_driver_name, drained - written);

    return written;
}

static int fb_sfxDriverTryFromIndex(int start_index)
{
    int i;

    if (!__fb_sfx)
        return -1;

    if (start_index < 0)
        start_index = 0;

    for (i = start_index; __fb_sfx_drivers_list[i]; ++i)
    {
        const SFXDRIVER *driver = __fb_sfx_drivers_list[i];

        if (!driver || !driver->init)
            continue;

        if (fb_sfxDriverValidate(driver) != 0)
            continue;

        SFX_DEBUG("sfx_core: attempting driver '%s'", driver->name);

        if (driver->init(
                __fb_sfx->samplerate,
                __fb_sfx->output_channels,
                __fb_sfx->buffer_size,
                FB_SFX_INIT_DEFAULT) == 0)
        {
            __fb_sfx->driver = driver;
            fb_sfxDriverStatsReset(driver->name);

            SFX_DEBUG("sfx_core: driver '%s' initialized", driver->name);

            return 0;
        }
    }

    __fb_sfx->driver = NULL;
    return -1;
}

int fb_sfxDriverValidate(const SFXDRIVER *driver)
{
    if (!driver)
        return -1;

    if (!driver->name || !driver->init || !driver->exit || !driver->write)
    {
        SFX_DEBUG("sfx_core: driver has an incomplete lifecycle/write table");
        return -1;
    }

    if ((driver->capabilities & FB_SFX_DRIVER_CAP_CAPTURE) &&
        !driver->capture_read)
    {
        SFX_DEBUG("sfx_core: driver '%s' advertises capture without capture_read",
                  driver->name);
        return -1;
    }

    if (driver->capabilities & FB_SFX_DRIVER_CAP_MIDI)
    {
        /*
            MIDI is currently routed through the separate platform MIDI
            subsystem, not through SFXDRIVER callbacks.  Reject this flag here
            so the driver table cannot describe a hook that the interface does
            not actually expose.
        */
        SFX_DEBUG("sfx_core: driver '%s' advertises MIDI through SFXDRIVER without MIDI hooks",
                  driver->name);
        return -1;
    }

    return 0;
}

static int fb_sfxDriverTryByName(const char *name)
{
    int i;

    if (!__fb_sfx || !name || !*name)
        return -1;

    for (i = 0; __fb_sfx_drivers_list[i]; ++i)
    {
        const SFXDRIVER *driver = __fb_sfx_drivers_list[i];

        if (!driver || !driver->name || !driver->init)
            continue;

        if (!fb_sfxDriverNameEquals(driver->name, name))
            continue;

        if (fb_sfxDriverValidate(driver) != 0)
        {
            SFX_DEBUG("sfx_core: requested driver '%s' has invalid capabilities",
                      driver->name);
            return -1;
        }

        SFX_DEBUG("sfx_core: attempting requested driver '%s'", driver->name);

        if (driver->init(
                __fb_sfx->samplerate,
                __fb_sfx->output_channels,
                __fb_sfx->buffer_size,
                FB_SFX_INIT_DEFAULT) == 0)
        {
            __fb_sfx->driver = driver;
            fb_sfxDriverStatsReset(driver->name);

            SFX_DEBUG("sfx_core: requested driver '%s' initialized",
                      driver->name);

            return 0;
        }

        SFX_DEBUG("sfx_core: requested driver '%s' failed", driver->name);
        return -1;
    }

    SFX_DEBUG("sfx_core: requested driver '%s' was not found", name);
    return -1;
}

static int fb_sfxDriverFallback(const SFXDRIVER *failed_driver)
{
    const SFXDRIVER *driver_to_exit;
    int next_index;

    if (!__fb_sfx || !failed_driver)
        return -1;

    if (__fb_sfx->shutting_down)
        return -1;

    next_index = fb_sfxDriverIndexOf(failed_driver);
    if (next_index < 0)
        return -1;

    driver_to_exit = fb_sfxDriverDetachLocked(failed_driver);
    if (!driver_to_exit)
        return -1;

    fb_sfxRuntimeUnlock();
    fb_sfxDriverExitUnlocked(driver_to_exit);
    fb_sfxRuntimeLock();

    SFX_DEBUG("sfx_core: driver '%s' failed, trying fallback", failed_driver->name);

    return fb_sfxDriverTryFromIndex(next_index + 1);
}

/*
    fb_sfxDriverInit()

    Attempt to initialize drivers in the registered driver list.

    Drivers are attempted in order until one successfully
    initializes.
*/

int fb_sfxDriverInit(void)
{
    const char *requested_driver;
    int result;

    fb_sfxRuntimeLock();

    /*
        SFXLIB_DRIVER gives tests and demos a way to choose a backend before
        the first sound command initializes the subsystem.  This is especially
        useful for "null" diagnostic runs that should not open the user's real
        audio device.
    */

    requested_driver = getenv("SFXLIB_DRIVER");
    if (requested_driver &&
        *requested_driver &&
        !fb_sfxDriverNameEquals(requested_driver, "default"))
    {
        result = fb_sfxDriverTryByName(requested_driver);
        if (result == 0)
        {
            fb_sfxRuntimeUnlock();
            return 0;
        }
    }

    result = fb_sfxDriverTryFromIndex(0);
    fb_sfxRuntimeUnlock();

    return result;
}


/*
    fb_sfxDriverShutdown()

    Shutdown the active audio driver.
*/

void fb_sfxDriverShutdown(void)
{
    const SFXDRIVER *driver;

    fb_sfxRuntimeLock();

    if (!__fb_sfx)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    driver = fb_sfxDriverDetachLocked(NULL);
    fb_sfxRuntimeUnlock();

    fb_sfxDriverExitUnlocked(driver);
}

int fb_sfxDriverFeedsAudio(void)
{
    int result;

    result = 0;

    fb_sfxRuntimeLock();

    if (__fb_sfx && __fb_sfx->driver &&
        (__fb_sfx->driver->capabilities & FB_SFX_DRIVER_CAP_BACKGROUND))
    {
        result = 1;
    }

    fb_sfxRuntimeUnlock();

    return result;
}

void fb_sfxForegroundFeedBegin(void)
{
    fb_sfxRuntimeLock();
    g_foreground_feed_count++;
    fb_sfxRuntimeUnlock();
}

void fb_sfxForegroundFeedEnd(void)
{
    fb_sfxRuntimeLock();

    if (g_foreground_feed_count > 0)
        g_foreground_feed_count--;

    fb_sfxRuntimeUnlock();
}

int fb_sfxForegroundFeedActive(void)
{
    int result;

    fb_sfxRuntimeLock();
    result = (g_foreground_feed_count > 0);
    fb_sfxRuntimeUnlock();

    return result;
}

const SFXDRIVER *fb_sfxDriverDetachLocked(const SFXDRIVER *expected_driver)
{
    const SFXDRIVER *driver;

    if (!__fb_sfx)
        return NULL;

    driver = __fb_sfx->driver;
    if (!driver)
        return NULL;

    if (expected_driver && driver != expected_driver)
        return NULL;

    __fb_sfx->driver = NULL;
    return driver;
}

void fb_sfxDriverExitUnlocked(const SFXDRIVER *driver)
{
    if (driver && driver->exit)
    {
        fb_sfxDriverIoLock();
        driver->exit();
        fb_sfxDriverIoUnlock();
    }
}


/* end of sfx_core.c */
