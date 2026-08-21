/*
    FreeBASIC Windows CE locale driver
    ----------------------------------

    File: drv_intl_getmonthname.c

    Purpose:

        Return a localized full or abbreviated month name on Windows CE.

    Responsibilities:

        * validate the one-based month number
        * select the corresponding Windows locale information identifier
        * return the Coredll ANSI representation as a temporary FBSTRING

    This file intentionally does NOT contain:

        * desktop Windows console-code-page handling
        * date formatting or date arithmetic
        * locale selection policy
*/

#include "../fb.h"
#include "fb_private_intl.h"

/* ------------------------------------------------------------------------- */
/* Localized month-name retrieval                                             */
/* ------------------------------------------------------------------------- */

FBSTRING *fb_DrvIntlGetMonthName( int month, int short_names )
{
    LCTYPE locale_type;
    FBSTRING *result;
    char *name;
    size_t name_length;

    if( month < 1 || month > 12 )
        return NULL;

    if( short_names )
        locale_type = (LCTYPE)(LOCALE_SABBREVMONTHNAME1 + month - 1);
    else
        locale_type = (LCTYPE)(LOCALE_SMONTHNAME1 + month - 1);

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

/* end of drv_intl_getmonthname.c */
