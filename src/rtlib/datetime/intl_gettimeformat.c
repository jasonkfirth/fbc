/*
 * Project: FreeBASIC runtime library
 * File: datetime/intl_gettimeformat.c
 * Purpose: get short TIME format.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
int fb_IntlGetTimeFormat( char *buffer, size_t len, int disallow_localized )
{
    if( fb_I18nGet() && !disallow_localized ) {
        if( fb_DrvIntlGetTimeFormat( buffer, len ) )
            return TRUE;
    }
    if( len < 9 )
        return FALSE;
    memcpy( buffer, "HH:mm:ss", sizeof( "HH:mm:ss" ) );
    return TRUE;
}

/* end of datetime/intl_gettimeformat.c */
