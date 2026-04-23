/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_debug.h

    Purpose:

        Provide a centralized debugging interface for the Haiku
        graphics backend.

    Responsibilities:

        • declare debug helper functions
        • provide debug macros used across the backend
        • ensure consistent debug linkage between C and C++

    This file intentionally does NOT contain:

        • rendering logic
        • window management
        • event processing
*/

#ifndef FB_GFX_HAIKU_DEBUG_H
#define FB_GFX_HAIKU_DEBUG_H

/* ------------------------------------------------------------------------- */
/* C / C++ linkage control                                                   */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Debug interface                                                           */
/* ------------------------------------------------------------------------- */

/*
    Initializes debugging configuration.

    This checks the environment variable:

        HAIKU_GFX_DEBUG

    If the variable is set and non-zero, debug output is enabled.
*/

void fb_hHaikuInitDebug(void);


/*
    Returns non-zero if debug output is enabled.
*/

int fb_hHaikuDebugEnabled(void);


/* ------------------------------------------------------------------------- */
/* End C linkage block                                                       */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif


/* ------------------------------------------------------------------------- */
/* Debug macros                                                              */
/* ------------------------------------------------------------------------- */

#ifdef HAIKU_GFX_ENABLE_DEBUG

#include <stdio.h>

#define HAIKU_DEBUG(fmt, ...) \
    do { \
        if (fb_hHaikuDebugEnabled()) \
            fprintf(stderr, "HAIKU_GFX: " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#else

#define HAIKU_DEBUG(fmt, ...) \
    do { } while (0)

#endif


#endif

/* end of haiku_debug.h */
