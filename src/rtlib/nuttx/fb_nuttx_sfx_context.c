/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_sfx_context.c

    Purpose:

        Provide the normal rtlib context object for NuttX tests that link the
        real sfxlib runtime without also linking gfxlib2.

    Responsibilities:

        - provide the shared __fb_ctx object used by sfxlib initialization
        - provide the shared error message buffer expected by rtlib headers
        - keep pure-sfx NuttX smoke apps from depending on gfxlib glue

    This file intentionally does NOT contain:

        - sound command implementations
        - audio driver logic
        - graphics compatibility code
        - program startup or shutdown policy
*/

#include "rtlib/fb.h"

/*
    NuttX flat builds link all enabled builtin apps into one executable.
    During bring-up, separate gfxlib and sfxlib smoke apps can each bring in a
    small runtime bridge, so these shared rtlib globals must coalesce at link
    time.  The permanent NuttX rtlib should eventually provide one strong
    definition instead.
*/
#define FB_NUTTX_WEAK __attribute__((weak))

FB_RTLIB_CTX __fb_ctx FB_NUTTX_WEAK;
char __fb_errmsg[FB_ERRMSG_SIZE] FB_NUTTX_WEAK;

/* end of fb_nuttx_sfx_context.c */
