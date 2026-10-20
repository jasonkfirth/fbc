/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/drv_intl_getmonthname.c
 * Purpose: get localized month name.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBSTRING *fb_DrvIntlGetMonthName( int month, int short_names )
{
    /* No localized month name available */
    return NULL;
}

/* end of drivers/xbox/drv_intl_getmonthname.c */
