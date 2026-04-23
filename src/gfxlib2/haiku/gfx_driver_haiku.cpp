/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: gfx_driver_haiku.cpp

    Purpose:

        Define the Haiku graphics driver entry used by the FreeBASIC
        graphics runtime.
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"
#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int haiku_driver_debug_enabled(void)
{
    const char *e = getenv("HAIKU_GFX_DEBUG");
    return (e && *e && *e != '0');
}

#define HAIKU_DRIVER_DEBUG(...) \
    do { \
        if (haiku_driver_debug_enabled()) \
            fprintf(stderr, "HAIKU_GFX: " __VA_ARGS__); \
    } while (0)


/* ------------------------------------------------------------------------- */
/* Driver initialization wrapper                                             */
/* ------------------------------------------------------------------------- */

static int driver_init(char *title,
                       int w,
                       int h,
                       int depth,
                       int refresh_rate,
                       int flags)
{
    HAIKU_DRIVER_DEBUG(
        "driver_init(title=%s, w=%d, h=%d, depth=%d, refresh=%d, flags=%d)\n",
        title ? title : "(null)",
        w,
        h,
        depth,
        refresh_rate,
        flags
    );

    return fb_hHaikuInit(title, w, h, depth, refresh_rate, flags);
}


/* ------------------------------------------------------------------------- */
/* Driver export                                                             */
/* ------------------------------------------------------------------------- */

extern "C" const GFXDRIVER fb_gfxDriverHaiku =
{
    (char*)"Haiku",

    driver_init,
    fb_hHaikuExit,

    fb_hHaikuLock,
    fb_hHaikuUnlock,

    fb_hHaikuSetPalette,
    fb_hHaikuWaitVSync,

    fb_hHaikuGetMouse,
    fb_hHaikuSetMouse,

    fb_hHaikuSetWindowTitle,
    fb_hHaikuSetWindowPos,

    fb_hHaikuFetchModes,

    NULL,

    fb_hHaikuPollEvents,
    fb_hHaikuUpdate
};

#endif

/* end of gfx_driver_haiku.cpp */
