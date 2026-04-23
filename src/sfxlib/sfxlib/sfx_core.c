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

#if defined(__linux__) && !defined(DISABLE_LINUX)
#include "linux/fb_sfx_linux.h"
#endif

static void fb_sfxInitCoreRollbackLocked(void);

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

#if defined(__linux__) && !defined(DISABLE_LINUX)
    fb_sfxLinuxExit();
#endif

    fb_sfxCaptureShutdown();

    fb_sfxMixBufferShutdown();
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
    float *buffer;
    const SFXDRIVER *driver;
    int frames_remaining;
    int frames_this_pass;
    int max_frames;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();

    if (frames <= 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    buffer = __fb_sfx->mixbuffer;

    if (buffer == NULL)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    max_frames = (__fb_sfx->buffer_frames > 0)
        ? __fb_sfx->buffer_frames
        : frames;
    if (max_frames <= 0)
        max_frames = frames;

    frames_remaining = frames;

    while (frames_remaining > 0)
    {
        frames_this_pass = (frames_remaining > max_frames)
            ? max_frames
            : frames_remaining;

        /* generate audio using the mixer */
        fb_sfxMixerProcess(frames_this_pass);

        /* mix decoded playback sources into the same live output buffer */
        fb_sfxMixFeedSource(frames_this_pass, fb_sfxAudioFeed);
        fb_sfxMixFeedSource(frames_this_pass, fb_sfxStreamFeed);

        /* send samples to the active driver */
        driver = __fb_sfx->driver;

        while (driver && driver->write)
        {
            if (driver->write(buffer, frames_this_pass) >= 0)
                break;

            if (fb_sfxDriverFallback(driver) != 0)
                break;

            driver = __fb_sfx->driver;
        }

        frames_remaining -= frames_this_pass;
    }

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
