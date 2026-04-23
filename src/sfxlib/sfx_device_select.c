/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_device_select.c

    Purpose:

        Implement the DEVICE SELECT command.

        This command selects which audio backend driver should be
        used by the sfxlib runtime.

    Responsibilities:

        • select an audio driver by index
        • safely shut down any currently running driver
        • initialize the newly selected driver
        • update runtime driver state

    This file intentionally does NOT contain:

        • audio mixing logic
        • driver implementations
        • command parsing
        • device enumeration

    Architectural overview:

        DEVICE SELECT
              │
        choose driver from registry
              │
        shutdown current driver
              │
        initialize new driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* External driver registry                                                  */
/* ------------------------------------------------------------------------- */

extern const FB_SFX_DRIVER *__fb_sfx_drivers_list[];


/* ------------------------------------------------------------------------- */
/* Current driver state                                                      */
/* ------------------------------------------------------------------------- */

static const FB_SFX_DRIVER *g_current_driver = NULL;
static int g_current_device_id = -1;


/* ------------------------------------------------------------------------- */
/* Driver lookup helpers                                                     */
/* ------------------------------------------------------------------------- */

static int fb_sfxDeviceDriverIndexLocked(const FB_SFX_DRIVER *driver)
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

static const FB_SFX_DRIVER *fb_sfxCurrentDriverLocked(void)
{
    if (__fb_sfx && __fb_sfx->driver)
        return __fb_sfx->driver;

    return g_current_driver;
}

static void fb_sfxSyncCurrentDriverLocked(void)
{
    const FB_SFX_DRIVER *driver;

    driver = fb_sfxCurrentDriverLocked();
    g_current_driver = driver;
    g_current_device_id = fb_sfxDeviceDriverIndexLocked(driver);
}


/* ------------------------------------------------------------------------- */
/* Helper: shutdown current driver                                           */
/* ------------------------------------------------------------------------- */

static void fb_sfxShutdownCurrentDriver(void)
{
    const FB_SFX_DRIVER *driver;

    driver = fb_sfxCurrentDriverLocked();
    if (!driver)
        goto clear_state;

clear_state:
    if (__fb_sfx)
        __fb_sfx->driver = NULL;

    g_current_driver = NULL;
    g_current_device_id = -1;

    /*
        Call the backend exit routine after dropping the runtime lock.
        Windows playback backends can own a feeder thread, and waiting for it
        while still holding the runtime lock can deadlock against that worker.
    */
    fb_sfxRuntimeUnlock();
    fb_sfxDriverExitUnlocked(driver);
    fb_sfxRuntimeLock();
}


/* ------------------------------------------------------------------------- */
/* Select device                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceSelect()

    Select an audio driver by index.
*/

int fb_sfxDeviceSelect(int id)
{
    const FB_SFX_DRIVER *drv;
    const FB_SFX_DRIVER *current;
    int samplerate;
    int channels;
    int buffer_size;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (id < 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    drv = __fb_sfx_drivers_list[id];

    if (!drv)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    fb_sfxSyncCurrentDriverLocked();
    current = g_current_driver;
    samplerate = __fb_sfx ? __fb_sfx->samplerate : FB_SFX_DEFAULT_RATE;
    channels = __fb_sfx ? __fb_sfx->output_channels : FB_SFX_DEFAULT_CHANNELS;
    buffer_size = __fb_sfx ? __fb_sfx->buffer_size : FB_SFX_DEFAULT_BUFFER;

    if (current == drv)
    {
        SFX_DEBUG("sfx_device_select: audio device already selected: %s", drv->name);
        fb_sfxRuntimeUnlock();
        return 0;
    }

    /*
        Shut down currently active driver
    */

    fb_sfxShutdownCurrentDriver();

    /*
        Attempt to initialize new driver
    */

    if (drv->init)
    {
        if (drv->init(
                samplerate,
                channels,
                buffer_size,
                FB_SFX_INIT_DEFAULT) != 0)
        {
            SFX_DEBUG("sfx_device_select: failed to initialize '%s'", drv->name);

            if (current && current->init &&
                current->init(samplerate, channels, buffer_size, FB_SFX_INIT_DEFAULT) == 0)
            {
                __fb_sfx->driver = current;
                fb_sfxSyncCurrentDriverLocked();
                SFX_DEBUG("sfx_device_select: restored '%s' after failed selection",
                          current->name);
            }
            else
            {
                fb_sfxDriverInit();
                fb_sfxSyncCurrentDriverLocked();
                if (g_current_driver)
                {
                    SFX_DEBUG("sfx_device_select: fell back to '%s' after failed selection",
                              g_current_driver->name);
                }
                else
                {
                    SFX_DEBUG("sfx_device_select: no driver available after failed selection");
                }
            }

            fb_sfxRuntimeUnlock();
            SFX_DEBUG("sfx_device_select: failed to initialize audio device: %s",
                      drv->name);
            return -1;
        }
    }

    g_current_driver = drv;
    g_current_device_id = id;
    __fb_sfx->driver = drv;

    SFX_DEBUG("sfx_device_select: audio device selected: %s", drv->name);

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Get current device                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceCurrent()

    Return the currently selected device index.
*/

int fb_sfxDeviceCurrent(void)
{
    int current_device_id;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    fb_sfxSyncCurrentDriverLocked();
    current_device_id = g_current_device_id;
    fb_sfxRuntimeUnlock();

    return current_device_id;
}


/* ------------------------------------------------------------------------- */
/* Get current driver                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceDriver()

    Return the currently active driver.
*/

const FB_SFX_DRIVER *fb_sfxDeviceDriver(void)
{
    const FB_SFX_DRIVER *driver;

    if (!fb_sfxEnsureInitialized())
        return NULL;

    fb_sfxRuntimeLock();
    fb_sfxSyncCurrentDriverLocked();
    driver = g_current_driver;
    fb_sfxRuntimeUnlock();

    return driver;
}


/* ------------------------------------------------------------------------- */
/* Shutdown device                                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceShutdown()

    Shut down the active audio driver.
*/

void fb_sfxDeviceShutdown(void)
{
    fb_sfxRuntimeLock();
    fb_sfxShutdownCurrentDriver();
    fb_sfxRuntimeUnlock();
}


/* end of sfx_device_select.c */
