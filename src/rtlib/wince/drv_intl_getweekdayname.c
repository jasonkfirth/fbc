/*
    FreeBASIC Windows CE locale driver
    ----------------------------------

    File: drv_intl_getweekdayname.c

    Purpose:

        Return a localized full or abbreviated weekday name on Windows CE.

    Responsibilities:

        * validate the one-based FreeBASIC weekday number
        * map Sunday-first numbering to Windows locale identifiers
        * return the Coredll ANSI representation as a temporary FBSTRING

    This file intentionally does NOT contain:

        * desktop Windows console-code-page handling
        * date formatting or date arithmetic
        * locale selection policy
*/

#include "../fb.h"
#include "fb_private_intl.h"

/* ------------------------------------------------------------------------- */
/* Localized weekday-name retrieval                                           */
/* ------------------------------------------------------------------------- */

FBSTRING *fb_DrvIntlGetWeekdayName( int weekday, int short_names )
{
    LCTYPE locale_type;
    FBSTRING *result;
    char *name;
    size_t name_length;

    if( weekday < 1 || weekday > 7 )
        return NULL;

    /* Windows numbers Monday as locale day one; FreeBASIC starts on Sunday. */
    if( weekday==1 )
        weekday = 8;

    if( short_names )
        locale_type = (LCTYPE)(LOCALE_SABBREVDAYNAME1 + weekday - 2);
    else
        locale_type = (LCTYPE)(LOCALE_SDAYNAME1 + weekday - 2);

    name = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT,
                              locale_type,
                              NULL,
                              0 );
    if( name==NULL )
        return NULL;

    name_length = strlen( name );
    result = fb_hStrAllocTemp( NULL, name_length );

    if( result!=NULL )
        FB_MEMCPY( result->data, name, name_length + 1 );

    free( name );
    return result;
}

/* end of drv_intl_getweekdayname.c */
