/*
 * Project: FreeBASIC runtime library
 * File: drivers/wii/drv_intl_gettimeformat.c
 * Purpose: Provide drv intl gettimeformat runtime support.
 * Responsibilities: Target-specific runtime support for wii.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime Wii time format hook
    --------------------------------------

    File: drv_intl_gettimeformat.c
*/

#include "../../fb.h"

int fb_DrvIntlGetTimeFormat(char *buffer, size_t len)
{
	return FALSE;
}

/* end of drivers/wii/drv_intl_gettimeformat.c */
