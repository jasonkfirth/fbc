/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/drv_intl_getweekdayname.c
 * Purpose: get localized weekday name.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"

FBSTRING_ACCESS *fb_DrvIntlGetWeekdayName( int weekday, int short_names )
{
    char *pszName = NULL;
    size_t name_len;
    LCTYPE lctype;
    FBSTRING_ACCESS *result;

    if( weekday < 1 || weekday > 7 )
        return NULL;

    if( weekday==1 )
        weekday = 8;

    if( short_names ) {
        lctype = (LCTYPE) (LOCALE_SABBREVDAYNAME1 + weekday - 2);
    } else {
        lctype = (LCTYPE) (LOCALE_SDAYNAME1 + weekday - 2);
    }

    pszName = fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, lctype,
                                 NULL, 0 );
    if( pszName==NULL )
        return NULL;

    name_len = strlen(pszName);
	if( name_len > (size_t)FB_STRSIZEMSK ) {
		free( pszName );
		return NULL;
	}

    result = fb_hStrAllocTemp( NULL, (ssize_t)name_len );
    if( result!=NULL ) {
        /* Use the process locale codepage selected by the Windows runtime. */
        int target_cp = (int)GetConsoleCP();
        if( target_cp!=-1 ) {
            FB_MEMCPY( result->data, pszName, name_len + 1 );
            result = fb_hIntlConvertString( result,
                                            CP_ACP,
                                            target_cp );
        }
    }

   free( pszName );

    return result;
}

/* end of drivers/win32/drv_intl_getweekdayname.c */
