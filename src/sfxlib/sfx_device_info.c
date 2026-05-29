/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_device_info.c

    Purpose:

        Implement the DEVICE INFO command.

        This command reports information about a specific
        audio device (driver) registered in the sfxlib
        driver table.

    Responsibilities:

        • query device information by index
        • return driver name
        • report whether the device is currently selected
        • provide a stable informational interface

    This file intentionally does NOT contain:

        • driver enumeration
        • driver initialization
        • playback logic
        • mixer logic

    Architectural overview:

        DEVICE INFO
              │
        driver registry lookup
              │
        device information output
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_driver_diag.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Device info                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceInfo()

    Print information about a specific device.
*/

void fb_sfxDeviceInfo(int id)
{
    const FB_SFX_DRIVER *drv;
    FB_SFX_DRIVER_STATS stats;
    int current;
    int i;

    if (id < 0)
    {
        SFX_DEBUG("sfx_device_info: invalid device index");
        return;
    }

    fb_sfxRuntimeLock();
    drv = __fb_sfx_drivers_list[id];

    if (!drv)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_device_info: device not found");
        return;
    }

    current = -1;
    if (__fb_sfx && __fb_sfx->driver)
    {
        for (i = 0; __fb_sfx_drivers_list[i]; ++i)
        {
            if (__fb_sfx_drivers_list[i] == __fb_sfx->driver)
            {
                current = i;
                break;
            }
        }
    }

    SFX_DEBUG("sfx_device_info: audio device information");
    SFX_DEBUG("sfx_device_info: index: %d", id);
    SFX_DEBUG("sfx_device_info: driver: %s", drv->name);

    if (current == id)
        SFX_DEBUG("sfx_device_info: status: active");
    else
        SFX_DEBUG("sfx_device_info: status: available");

    SFX_DEBUG("sfx_device_info: capabilities: output%s%s%s%s",
              (drv->capabilities & FB_SFX_DRIVER_CAP_CAPTURE) ? " capture" : "",
              (drv->capabilities & FB_SFX_DRIVER_CAP_MIDI) ? " midi" : "",
              (drv->capabilities & FB_SFX_DRIVER_CAP_BACKGROUND) ? " background" : "",
              (drv->capabilities & FB_SFX_DRIVER_CAP_BLOCKING) ? " blocking" : "");

    if (current == id && __fb_sfx)
    {
        SFX_DEBUG("sfx_device_info: format: rate=%d channels=%d buffer=%d",
                  __fb_sfx->samplerate,
                  __fb_sfx->output_channels,
                  __fb_sfx->buffer_size);
    }

    if (fb_sfxDriverStatsSnapshot(drv->name, &stats) == 0)
    {
        SFX_DEBUG("sfx_device_info: stats: writes=%llu requested=%llu accepted=%llu dropped=%llu",
                  stats.write_calls,
                  stats.frames_requested,
                  stats.frames_accepted,
                  stats.frames_dropped);
        SFX_DEBUG("sfx_device_info: stats: short=%llu zero=%llu errors=%llu underruns=%llu overruns=%llu recoveries=%llu",
                  stats.short_writes,
                  stats.zero_writes,
                  stats.errors,
                  stats.underruns,
                  stats.overruns,
                  stats.recoveries);
        SFX_DEBUG("sfx_device_info: stats: queue=%d max_queue=%d last_error=%d",
                  stats.current_queue_fill,
                  stats.max_queue_fill,
                  stats.last_error);
    }

    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Device name lookup                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceInfoName()

    Return the name of a device.
*/

const char *fb_sfxDeviceInfoName(int id)
{
    const char *name;

    if (id < 0)
        return NULL;

    fb_sfxRuntimeLock();
    if (!__fb_sfx_drivers_list[id])
    {
        fb_sfxRuntimeUnlock();
        return NULL;
    }

    name = __fb_sfx_drivers_list[id]->name;
    fb_sfxRuntimeUnlock();

    return name;
}


/* ------------------------------------------------------------------------- */
/* Device validity check                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceValid()

    Return non-zero if a device index is valid.
*/

int fb_sfxDeviceValid(int id)
{
    int valid;

    if (id < 0)
        return 0;

    fb_sfxRuntimeLock();
    valid = (__fb_sfx_drivers_list[id] != NULL);
    fb_sfxRuntimeUnlock();

    return valid;
}


/* end of sfx_device_info.c */
