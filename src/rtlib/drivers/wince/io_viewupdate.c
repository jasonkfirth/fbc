/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/io_viewupdate.c

    Purpose:

        Complete VIEW PRINT updates for the logical Windows CE console.

    Responsibilities:

        - preserve the platform hook expected by generic VIEW code

    This file intentionally does NOT contain: - desktop scroll-region configuration
        - native window invalidation
        - view-bound storage
*/

#include "../../fb.h"

void fb_ConsoleViewUpdate( void )
{
}

/* end of drivers/wince/io_viewupdate.c */
