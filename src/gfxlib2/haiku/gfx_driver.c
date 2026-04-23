/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: gfx_driver.c

    Purpose:

        Register the Haiku graphics driver with the FreeBASIC graphics
        runtime.

    Responsibilities:

        • expose the list of available graphics drivers
        • ensure the Haiku backend is available to gfxlib
        • define driver selection order

    This file intentionally does NOT contain:

        • rendering logic
        • window creation
        • event processing
        • platform initialization code

    Design notes:

        The FreeBASIC graphics runtime scans the driver list in order.
        The first driver whose initialization routine succeeds becomes
        the active graphics backend.

        For the Haiku build, the driver list contains:

            1. The Haiku graphics backend
            2. The fallback null driver

        The null driver ensures the runtime can continue operating even
        if graphics initialization fails.
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"
#include "fb_gfx_haiku.h"


/* ------------------------------------------------------------------------- */
/* External driver declarations                                              */
/* ------------------------------------------------------------------------- */

/*
    The actual Haiku driver implementation is defined in
    gfx_driver_haiku.cpp.

    We declare it here so it can be added to the driver list.
*/

extern const GFXDRIVER fb_gfxDriverHaiku;


/* ------------------------------------------------------------------------- */
/* Driver list                                                               */
/* ------------------------------------------------------------------------- */

/*
    gfxlib scans this list in order and selects the first driver whose
    initialization function returns success.

    Order therefore determines driver priority.
*/

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverHaiku,   /* Primary Haiku graphics backend */
    &__fb_gfxDriverNull,  /* Fallback driver */
    NULL
};

#endif

/* end of gfx_driver.c */
