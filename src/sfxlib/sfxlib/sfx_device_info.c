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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* External driver registry                                                  */
/* ------------------------------------------------------------------------- */

extern const FB_SFX_DRIVER *__fb_sfx_drivers_list[];


/* ------------------------------------------------------------------------- */
/* External device selection state                                           */
/* ------------------------------------------------------------------------- */

extern int fb_sfxDeviceCurrent(void);


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
    int current;

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

    current = fb_sfxDeviceCurrent();

    SFX_DEBUG("sfx_device_info: audio device information");
    SFX_DEBUG("sfx_device_info: index: %d", id);
    SFX_DEBUG("sfx_device_info: driver: %s", drv->name);

    if (current == id)
        SFX_DEBUG("sfx_device_info: status: active");
    else
        SFX_DEBUG("sfx_device_info: status: available");
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
