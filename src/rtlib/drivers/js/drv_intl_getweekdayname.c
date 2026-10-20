/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/drv_intl_getweekdayname.c
 * Purpose: get localized weekday name.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBSTRING *fb_DrvIntlGetWeekdayName( int weekday, int short_names )
{
    /* No localized weekday name available */
    return NULL;
}

/* end of drivers/js/drv_intl_getweekdayname.c */
