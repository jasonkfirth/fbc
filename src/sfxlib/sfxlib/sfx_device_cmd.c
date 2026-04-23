/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_device_cmd.c

    Purpose:

        Provide command-facing helpers for DEVICE operations.

        DEVICE INFO is most useful as a command without an explicit
        identifier, meaning "show information for the currently
        selected device".  This helper keeps that policy out of the
        parser layer.

    Responsibilities:

        • adapt current-device queries to command form

    This file intentionally does NOT contain:

        • device enumeration
        • device selection
        • driver startup or shutdown
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* DEVICE command helpers                                                    */
/* ------------------------------------------------------------------------- */

void fb_sfxDeviceInfoCurrent(void)
{
    fb_sfxDeviceInfo(fb_sfxDeviceCurrent());
}

/* end of sfx_device_cmd.c */
