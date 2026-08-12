/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_driver.c

    Purpose:

        Register the RISC OS sfxlib driver list for the initial port.

    Responsibilities:

        - expose the driver-list symbol required by shared sfxlib code
        - select the portable null driver as a deterministic fallback

    This file intentionally does NOT contain:

        - SoundDMA or audio-module control
        - sample conversion
        - background playback or synchronization

    Driver behavior:

        The shared null driver accepts the normal sfxlib initialization path
        without claiming native audio support.  A SoundDMA driver can be added
        ahead of it later while retaining the null fallback.
*/

#include "../fb_sfx_driver.h"

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] = {
	&__fb_sfxDriverNull,
	0
};

/* end of sfx_driver.c */
