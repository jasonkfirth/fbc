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

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_mixer.h"
#include "fb_sfx_buffer.h"
#include "fb_sfx_capture.h"

static void fb_sfxInitCoreRollbackLocked(void);
static int fb_sfxOutputQueueInitLocked(void);
static void fb_sfxOutputQueueShutdownLocked(void);
static int fb_sfxOutputQueueFillLocked(int frames);
static int fb_sfxOutputQueueDrainLocked(int frames);


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
    fb_sfxRuntimeUnlock();

    /*
        MIDI playback may be running on a worker thread.

        Stop and join it before tearing down the driver layer so the
        worker cannot continue dispatching events into a closed backend.
    */

    fb_sfxMidiStop();

    fb_sfxDriverShutdown();

    fb_sfxRuntimeLock();

    fb_sfxPlatformExit();

    fb_sfxCaptureShutdown();

    fb_sfxMixBufferShutdown();
    fb_sfxOutputQueueShutdownLocked();
    fb_sfxCaptureBufferShutdown();

    fb_sfxMixerShutdown();

    __fb_sfx->initialized = 0;
    fb_sfxRuntimeUnlock();
}

static int fb_sfxDriverFallback(const SFXDRIVER *failed_driver);
static void fb_sfxMixFeedSource(int frames, int (*feed_fn)(float *buffer, int frames));

static float fb_sfxCoreClampSample(float v)
{
    if (v > 1.0f)
        return 1.0f;

    if (v < -1.0f)
        return -1.0f;

    return v;
}

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
    fb_sfxMixerShutdown();
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

    if (fb_sfxOutputQueueFillLocked(frames) != 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    written = fb_sfxOutputQueueDrainLocked(frames);
    (void)written;

    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Driver selection                                                          */
/* ------------------------------------------------------------------------- */

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
    int produced;
    int i;

    if (!__fb_sfx || !__fb_sfx->mixbuffer || !feed_fn || frames <= 0)
        return;

    channels = (__fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    scratch = (float *)calloc((size_t)frames * (size_t)channels, sizeof(float));
    if (!scratch)
        return;

    produced = feed_fn(scratch, frames);
    if (produced > frames)
        produced = frames;

    for (i = 0; i < produced * channels; ++i)
        __fb_sfx->mixbuffer[i] = fb_sfxCoreClampSample(__fb_sfx->mixbuffer[i] + scratch[i]);

    free(scratch);
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
    int channels;
    int queued;
    int drained;
    int written;

    if (!__fb_sfx || !__fb_sfx->mixbuffer || frames <= 0)
        return 0;

    if (!g_output_queue_initialized)
        return 0;

    queued = fb_sfxRingBufferAvailable(&g_output_queue);
    if (queued <= 0)
        return 0;

    if (frames > queued)
        frames = queued;

    drained = fb_sfxRingBufferRead(&g_output_queue, __fb_sfx->mixbuffer, frames);
    if (drained <= 0)
        return 0;

    channels = (__fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    driver = __fb_sfx->driver;
    written = 0;

    while (driver && driver->write)
    {
        int result;

        result = driver->write(__fb_sfx->mixbuffer + (written * channels),
                               drained - written);
        if (result > 0)
        {
            written += result;

            if (written >= drained)
                return written;

            continue;
        }
        else if (result == 0)
        {
            return written;
        }

        if (fb_sfxDriverFallback(driver) != 0)
            break;

        driver = __fb_sfx->driver;
    }

    return written;
}

static int fb_sfxDriverTryFromIndex(int start_index)
{
    int i;

    if (!__fb_sfx)
        return -1;

    for (i = start_index; __fb_sfx_drivers_list[i]; ++i)
    {
        const SFXDRIVER *driver = __fb_sfx_drivers_list[i];

        if (!driver || !driver->init)
            continue;

        SFX_DEBUG("sfx_core: attempting driver '%s'", driver->name);

        if (driver->init(
                __fb_sfx->samplerate,
                __fb_sfx->output_channels,
                __fb_sfx->buffer_size,
                FB_SFX_INIT_DEFAULT) == 0)
        {
            __fb_sfx->driver = driver;

            SFX_DEBUG("sfx_core: driver '%s' initialized", driver->name);

            return 0;
        }
    }

    __fb_sfx->driver = NULL;
    return -1;
}

static int fb_sfxDriverFallback(const SFXDRIVER *failed_driver)
{
    const SFXDRIVER *driver_to_exit;
    int next_index;

    if (!__fb_sfx || !failed_driver)
        return -1;

    next_index = fb_sfxDriverIndexOf(failed_driver);
    if (next_index < 0)
        return -1;

    driver_to_exit = fb_sfxDriverDetachLocked(failed_driver);
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
    int result;

    fb_sfxRuntimeLock();
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
        driver->exit();
}


/* end of sfx_core.c */
