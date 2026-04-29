/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide macOS-specific shared sfxlib teardown.
*/

#ifndef DISABLE_DARWIN
#include "fb_sfx_darwin.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_DARWIN
    fb_sfxDarwinExit();
#endif
}

/* end of sfx_platform.c */
