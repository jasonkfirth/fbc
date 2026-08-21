/*
    FreeBASIC Sound Library support for AROS
    ----------------------------------------

    File: sfx_driver.c

    Purpose:

        Register native AROS audio before the safe null fallback.

    Responsibilities:

        - expose the driver-list symbol required by shared sfxlib code
        - prefer ahi.device output
        - retain predictable operation when AHI is unavailable

    This file intentionally does NOT contain:

        - AHI request management
        - sample conversion
        - worker synchronization
*/

#include "../fb_sfx_driver.h"

#include <stddef.h>

extern const FB_SFX_DRIVER fb_sfxDriverArosAhi;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverArosAhi,
    &__fb_sfxDriverNull,
    NULL
};

/* end of sfx_driver.c */
