/*
    Project: FreeBASIC AROS runtime
    -------------------------------

    File: rtlib/aros/static/fbrt0.c

    Purpose:

        Start and stop the normal FreeBASIC runtime in AROS startup order.

    Responsibilities:

        - initialize rtlib after AROS has opened application libraries
        - initialize rtlib before any FreeBASIC global constructor
        - release rtlib after every FreeBASIC global destructor

    This file intentionally does NOT contain:

        - profiler initialization
        - general runtime implementation
        - architecture-specific AROS policy

    AROS collects .ctors separately and invokes that set in reverse order.
    A priority section therefore cannot guarantee that rtlib precedes plain
    FreeBASIC constructors.  The AROS INIT and EXIT symbol sets surround all
    constructor and destructor sets and provide the required lifecycle.
*/

#include "../../fb.h"

#include <aros/symbolsets.h>

/* ------------------------------------------------------------------------- */
/* AROS runtime lifecycle                                                    */
/* ------------------------------------------------------------------------- */

static int fb_hDoInit(void)
{
    fb_hRtInit();
    return 1;
}

static void fb_hDoExit(void)
{
    fb_hRtExit();
}

/* Run after the C library's INIT entries but before every constructor set. */
ADD2INIT(fb_hDoInit, 10000)

/* EXIT runs in reverse, so this executes before the C library is closed. */
ADD2EXIT(fb_hDoExit, 10000)

/* end of rtlib/aros/static/fbrt0.c */
