/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide FreeBSD-specific shared sfxlib teardown.
*/

#ifndef DISABLE_FREEBSD
#include "fb_sfx_freebsd.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_FREEBSD
    fb_sfxFreebsdExit();
#endif
}

/* end of sfx_platform.c */
