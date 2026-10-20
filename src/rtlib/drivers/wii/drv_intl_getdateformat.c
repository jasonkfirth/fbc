/*
 * Project: FreeBASIC runtime library
 * File: drivers/wii/drv_intl_getdateformat.c
 * Purpose: Provide drv intl getdateformat runtime support.
 * Responsibilities: Target-specific runtime support for wii.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime Wii date format hook
    --------------------------------------

    File: drv_intl_getdateformat.c
*/

#include "../../fb.h"

int fb_DrvIntlGetDateFormat(char *buffer, size_t len)
{
	return FALSE;
}

/* end of drivers/wii/drv_intl_getdateformat.c */
