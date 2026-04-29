/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide Haiku-specific shared sfxlib teardown.
*/

#ifndef DISABLE_HAIKU
#include "fb_sfx_haiku.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_HAIKU
    fb_sfxHaikuExit();
#endif
}

/* end of sfx_platform.c */
