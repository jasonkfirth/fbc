/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the RISC OS platform teardown hook required by sfxlib.

    Responsibilities:

        - stop the RISC OS audio worker before device and mixer teardown

    This file intentionally does NOT contain:

        - worker implementation
        - device configuration
        - PCM conversion
        - portable sound runtime logic
*/

#include "fb_sfx_riscos.h"

void fb_sfxPlatformExit(void)
{
    fb_sfxRiscosWorkerStop();
}

/* end of sfx_platform.c */
