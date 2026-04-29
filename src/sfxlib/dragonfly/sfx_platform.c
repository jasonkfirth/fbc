/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide DragonFly BSD-specific shared sfxlib teardown.
*/

#ifndef DISABLE_DRAGONFLY
#include "fb_sfx_dragonfly.h"
#endif

void fb_sfxPlatformExit(void)
{
#ifndef DISABLE_DRAGONFLY
    fb_sfxDragonflyExit();
#endif
}

/* end of sfx_platform.c */
