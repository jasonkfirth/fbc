/*
    Project: FreeBASIC AROS runtime
    -------------------------------

    File: rtlib/aros/static/fbrt1.c

    Purpose:

        Start and stop the call profiler and FreeBASIC runtime in AROS order.

    Responsibilities:

        - initialize rtlib before the call profiler
        - make both services available to FreeBASIC global constructors
        - stop the profiler before releasing rtlib

    This file intentionally does NOT contain:

        - cycle-counter profiler initialization
        - general runtime or profiler implementation
        - architecture-specific AROS policy

    AROS INIT and EXIT sets surround the constructor lifecycle.  They avoid
    the reverse .ctors ordering that otherwise starts rtlib too late.
*/

#include "../../fb.h"
#include "../../fb_profile.h"

#include <aros/symbolsets.h>

/* ------------------------------------------------------------------------- */
/* AROS profiled runtime lifecycle                                           */
/* ------------------------------------------------------------------------- */

static int fb_hDoInit(void)
{
    fb_hRtInit();
    fb_InitProfile();
    return 1;
}

static void fb_hDoExit(void)
{
    fb_EndProfile(0);
    fb_hRtExit();
}

ADD2INIT(fb_hDoInit, 10000)
ADD2EXIT(fb_hDoExit, 10000)

/* end of rtlib/aros/static/fbrt1.c */
