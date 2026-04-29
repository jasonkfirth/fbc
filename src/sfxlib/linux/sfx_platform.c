/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide Linux-specific shared backend teardown for sfxlib.
*/

#ifndef DISABLE_LINUX
#include "fb_sfx_linux.h"
#endif

#ifndef DISABLE_UNIX
#include "../unix/fb_sfx_unix.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_LINUX
    fb_sfxLinuxExit();
#endif
#ifndef DISABLE_UNIX
    fb_sfxUnixExit();
#endif
}

/* end of sfx_platform.c */
