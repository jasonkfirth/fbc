/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_driver.c

    Purpose:

        Register the Windows CE audio drivers in preference order.

    Responsibilities:

        - select WinMM waveform output as the native backend
        - retain the null driver as a deterministic fallback

    This file intentionally does NOT contain:

        - device initialization
        - audio mixing
        - MIDI transport or synthesis
*/

#include "fb_sfx_wince.h"

#include <stddef.h>

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverWinMM,
    &__fb_sfxDriverNull,
    NULL
};

/* end of sfx_driver.c */
