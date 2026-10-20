/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/sys_beep.c
 * Purpose: Provide sys beep runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/sys_beep.c

    Purpose:

        Provide the BEEP runtime hook on Windows CE devices without a
        desktop console speaker API.
*/

#include "../../fb.h"

FBCALL void fb_Beep( void )
{
}

/* end of drivers/wince/sys_beep.c */
