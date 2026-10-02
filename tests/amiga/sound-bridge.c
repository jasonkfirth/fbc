/*
    FreeBASIC classic AmigaOS sound qualification
    --------------------------------------------
    File: sound-bridge.c
    Purpose: Ensure qualification uses a native device rather than fallback.
    Responsibilities: Identify the selected backend while holding its lock.
    This file intentionally does NOT contain sound generation or capture.
*/

#include "../../src/sfxlib/fb_sfx_internal.h"
#include <string.h>

int amiga_test_sound_driver(void)
{
    int result = 0;
    fb_sfxRuntimeLock();
    if (__fb_sfx != NULL && __fb_sfx->initialized && __fb_sfx->driver != NULL) {
        if (strcmp(__fb_sfx->driver->name, "Amiga Paula") == 0) result = 1;
        else if (strcmp(__fb_sfx->driver->name, "AMIGA AHI") == 0) result = 2;
    }
    fb_sfxRuntimeUnlock();
    return result;
}

/* end of sound-bridge.c */
