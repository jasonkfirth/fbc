/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the default platform teardown hook for targets that do not
        need shared backend cleanup beyond the active driver exit callback.
*/

#include "fb_sfx_internal.h"

void fb_sfxPlatformExit(void)
{
}

/* end of sfx_platform.c */
