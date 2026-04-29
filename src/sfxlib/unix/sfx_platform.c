/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide shared Unix sfxlib teardown for Unix-family targets
        without a more specific target hook.
*/

#ifndef DISABLE_UNIX
#include "fb_sfx_unix.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_UNIX
    fb_sfxUnixExit();
#endif
}

/* end of sfx_platform.c */
