/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide OpenBSD-specific shared sfxlib teardown.
*/

#ifndef DISABLE_OPENBSD
#include "fb_sfx_openbsd.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_OPENBSD
    fb_sfxOpenbsdExit();
#endif
}

/* end of sfx_platform.c */
