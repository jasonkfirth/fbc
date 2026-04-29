/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide NetBSD-specific shared sfxlib teardown.
*/

#ifndef DISABLE_NETBSD
#include "fb_sfx_netbsd.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_NETBSD
    fb_sfxNetbsdExit();
#endif
}

/* end of sfx_platform.c */
