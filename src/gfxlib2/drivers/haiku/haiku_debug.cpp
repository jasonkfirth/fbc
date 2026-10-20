/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_debug.cpp

    Purpose:

        Provide runtime debugging support for the Haiku graphics backend.

    Responsibilities:

        • initialize debug configuration
        • report whether debugging is enabled
        • provide a centralized location for backend debug state

    This file intentionally does NOT contain:

        • rendering logic
        • window management
        • event handling
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Debug state                                                               */
/* ------------------------------------------------------------------------- */

/*
    Debug initialization is intentionally lazy so that the environment
    variable is only checked once.
*/

static int g_debug_initialized = 0;
static int g_debug_enabled = 0;


/* ------------------------------------------------------------------------- */
/* Public debug API                                                          */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/*
    fb_hHaikuInitDebug()

    Initializes debug configuration by checking the environment variable:

        HAIKU_GFX_DEBUG

    If the variable is set and non-zero, debugging output becomes enabled.
*/

void fb_hHaikuInitDebug(void)
{
    if (g_debug_initialized)
        return;

    g_debug_initialized = 1;

    const char *env = getenv("HAIKU_GFX_DEBUG");

    if (env && *env && *env != '0')
        g_debug_enabled = 1;
}


/*
    fb_hHaikuDebugEnabled()

    Returns non-zero if debugging output is enabled.
*/

int fb_hHaikuDebugEnabled(void)
{
    if (!g_debug_initialized)
        fb_hHaikuInitDebug();

    return g_debug_enabled;
}

#ifdef __cplusplus
}
#endif

#endif

/* end of haiku_debug.cpp */
