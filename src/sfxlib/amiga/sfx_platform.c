/*
    FreeBASIC Sound Library support for AmigaOS
    ----------------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the AMIGA platform teardown hook required by sfxlib.

    Responsibilities:

        - stop the AHI feeder before core teardown releases shared state

    This file intentionally does NOT contain:

        - worker implementation
        - AHI request management
        - portable sound runtime logic
*/

#include "fb_sfx_amiga.h"

void fb_sfxPlatformExit(void)
{
    fb_sfxAmigaWorkerStop();
}

/* end of sfx_platform.c */
