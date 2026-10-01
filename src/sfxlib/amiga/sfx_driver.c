/*
    FreeBASIC Sound Library support for AmigaOS
    ----------------------------------------

    File: sfx_driver.c

    Purpose:

        Register native AmigaOS audio before the safe null fallback.

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

extern const FB_SFX_DRIVER fb_sfxDriverAmigaAhi;
extern const FB_SFX_DRIVER fb_sfxDriverAmigaPaula;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverAmigaAhi,
    &fb_sfxDriverAmigaPaula,
    &__fb_sfxDriverNull,
    NULL
};

/* end of sfx_driver.c */
