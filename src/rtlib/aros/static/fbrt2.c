/*
    Project: FreeBASIC AROS runtime
    -------------------------------

    File: rtlib/aros/static/fbrt2.c

    Purpose:

        Start and stop the cycle profiler and FreeBASIC runtime in AROS order.

    Responsibilities:

        - initialize rtlib before the cycle profiler
        - make both services available to FreeBASIC global constructors
        - stop cycle profiling before releasing rtlib

    This file intentionally does NOT contain:

        - call-graph profiler initialization
        - general runtime or profiler implementation
        - architecture-specific AROS policy

    AROS INIT and EXIT sets surround the constructor lifecycle.  They avoid
    the reverse .ctors ordering that otherwise starts rtlib too late.
*/

#include "../../fb.h"
#include "../../fb_profile.h"

#include <aros/symbolsets.h>

/* ------------------------------------------------------------------------- */
/* AROS cycle-profiled runtime lifecycle                                     */
/* ------------------------------------------------------------------------- */

static int fb_hDoInit(void)
{
    fb_hRtInit();
    fb_InitProfileCycles();
    return 1;
}

static void fb_hDoExit(void)
{
    fb_EndProfileCycles(0);
    fb_hRtExit();
}

ADD2INIT(fb_hDoInit, 10000)
ADD2EXIT(fb_hDoExit, 10000)

/* end of rtlib/aros/static/fbrt2.c */
