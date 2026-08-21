/*
    FreeBASIC Sound Library support for AROS
    ----------------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the AROS platform teardown hook required by sfxlib.

    Responsibilities:

        - stop the AHI feeder before core teardown releases shared state

    This file intentionally does NOT contain:

        - worker implementation
        - AHI request management
        - portable sound runtime logic
*/

#include "fb_sfx_aros.h"

void fb_sfxPlatformExit(void)
{
    fb_sfxArosWorkerStop();
}

/* end of sfx_platform.c */
