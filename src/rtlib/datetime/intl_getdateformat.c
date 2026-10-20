/*
 * Project: FreeBASIC runtime library
 * File: datetime/intl_getdateformat.c
 * Purpose: get short DATE format.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
int fb_IntlGetDateFormat( char *buffer, size_t len, int disallow_localized )
{
    if( fb_I18nGet( ) && !disallow_localized ) {
        if( fb_DrvIntlGetDateFormat( buffer, len ) )
            return TRUE;
    }
    if( len < 11 )
        return FALSE;
    memcpy(buffer, "MM/dd/yyyy", 11);
    return TRUE;
}

/* end of datetime/intl_getdateformat.c */
