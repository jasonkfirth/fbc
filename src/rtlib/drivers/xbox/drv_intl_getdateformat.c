/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/drv_intl_getdateformat.c
 * Purpose: get localized short DATE format.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_DrvIntlGetDateFormat( char *buffer, size_t len )
{
    /* No localized date format available! */
    return FALSE;
}

/* end of drivers/xbox/drv_intl_getdateformat.c */
