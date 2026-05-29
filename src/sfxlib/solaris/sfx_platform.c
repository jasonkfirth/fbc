/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide Solaris/illumos-specific sfxlib teardown.

    Responsibilities:

        - shut down the Solaris/illumos audio backend state

    This file intentionally does NOT contain:

        - audio device opening
        - mixer implementation
        - driver registration
*/

#include "fb_sfx_solaris.h"

void fb_sfxPlatformExit(void)
{
    fb_sfxSolarisExit();
}

/* end of sfx_platform.c */
