/*
 * Project: FreeBASIC runtime library
 * File: datetime/intl_getset.c
 * Purpose: turns internationalization on/off and queries status.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static int intl_on = TRUE;

/*:::::*/
FBCALL void fb_I18nSet( int on_off )
{
    intl_on = on_off!=0;
}

/*:::::*/
FBCALL int fb_I18nGet( void )
{
    return intl_on;
}

/* end of datetime/intl_getset.c */
