/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_device_list.c

    Purpose:

        Implement the DEVICE LIST command.

        This command enumerates the available audio drivers that
        were compiled into the runtime and reports them to the user.

    Responsibilities:

        • iterate through the registered sfxlib driver list
        • print driver names in a predictable order
        • provide a programmatic device count
        • provide driver lookup helpers

    This file intentionally does NOT contain:

        • driver initialization
        • driver selection logic
        • audio playback
        • mixer logic

    Architectural overview:

        DEVICE LIST
              │
        enumerate driver table
              │
        return device list to program
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

/*
    The driver registry is provided by platform driver modules.
*/

extern const FB_SFX_DRIVER *__fb_sfx_drivers_list[];


/* ------------------------------------------------------------------------- */
/* Device count                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceCount()

    Return the number of compiled-in audio drivers.
*/

int fb_sfxDeviceCount(void)
{
    int count = 0;

    fb_sfxRuntimeLock();
    while (__fb_sfx_drivers_list[count])
        count++;
    fb_sfxRuntimeUnlock();

    return count;
}


/* ------------------------------------------------------------------------- */
/* Device name lookup                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceName()

    Return the driver name for a device index.
*/

const char *fb_sfxDeviceName(int id)
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
/* Device list output                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceList()

    Print the list of available drivers.

    This function is used by the BASIC command:

        DEVICE LIST
*/

void fb_sfxDeviceList(void)
{
    int i;
    int count;

    count = fb_sfxDeviceCount();

    SFX_DEBUG("sfx_device_list: available audio devices:");

    if (count == 0)
    {
        SFX_DEBUG("sfx_device_list:   (none)");
        return;
    }

    for (i = 0; i < count; i++)
    {
        const FB_SFX_DRIVER *drv = __fb_sfx_drivers_list[i];

        if (!drv)
            continue;

        SFX_DEBUG("sfx_device_list:   %d: %s", i, drv->name);
    }
}


/* ------------------------------------------------------------------------- */
/* Device lookup by name                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceFind()

    Locate a driver index by name.
*/

int fb_sfxDeviceFind(const char *name)
{
    int i;
    int id = -1;

    if (!name)
        return -1;

    fb_sfxRuntimeLock();
    for (i = 0; __fb_sfx_drivers_list[i]; i++)
    {
        const FB_SFX_DRIVER *drv = __fb_sfx_drivers_list[i];

        if (!drv || !drv->name)
            continue;

        if (strcmp(drv->name, name) == 0)
        {
            id = i;
            break;
        }
    }
    fb_sfxRuntimeUnlock();

    return id;
}


/* ------------------------------------------------------------------------- */
/* Device information                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceListInfo()

    Print basic information about a specific device.
*/

void fb_sfxDeviceListInfo(int id)
{
    const FB_SFX_DRIVER *drv;

    fb_sfxRuntimeLock();
    drv = __fb_sfx_drivers_list[id];

    if (!drv)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_device_list: invalid device");
        return;
    }

    SFX_DEBUG("sfx_device_list: device %d", id);
    SFX_DEBUG("sfx_device_list:   driver: %s", drv->name);
    fb_sfxRuntimeUnlock();
}


/* end of sfx_device_list.c */
