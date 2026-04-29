/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide Android-specific shared sfxlib teardown.
*/

#include "fb_sfx_android.h"

void fb_sfxPlatformExit(void)
{
    fb_hAndroidSfxSetLifecycle(0, 0);
}

/* end of sfx_platform.c */
