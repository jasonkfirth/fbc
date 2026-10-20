/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/drv_intl_gettimeformat.c
 * Purpose: get localized short TIME format.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_DrvIntlGetTimeFormat( char *buffer, size_t len )
{
    /* No localized time format available! */
    return FALSE;
}

/* end of drivers/xbox/drv_intl_gettimeformat.c */
