/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_driver.c

    Purpose:

        Register the RISC OS sfxlib driver list.

    Responsibilities:

        - expose the driver-list symbol required by shared sfxlib code
        - try native DigitalRenderer output before portable fallbacks
        - retain the null driver for machines without usable audio support

    This file intentionally does NOT contain:

        - DigitalRenderer or SoundDMA control
        - sample conversion
        - background playback or synchronization

    Driver behavior:

        The native driver uses GCCSDK UnixLib's /dev/dsp implementation, which
        loads DigitalRenderer and feeds the RISC OS SoundDMA subsystem.  The
        shared null driver remains last so sound commands degrade safely when
        the module or audio hardware is unavailable.
*/

#include "../fb_sfx_driver.h"

#include <stddef.h>

extern const FB_SFX_DRIVER fb_sfxDriverRiscosDsp;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] = {
    &fb_sfxDriverRiscosDsp,
    &__fb_sfxDriverNull,
    NULL
};

/* end of sfx_driver.c */
