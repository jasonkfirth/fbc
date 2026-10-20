/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/drv_intl_getdateformat.c
 * Purpose: get localized short DATE format.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"

int fb_DrvIntlGetDateFormat( char *buffer, size_t len )
{
    DOS_COUNTRY_INFO_GENERAL Info;

    if( len < 11 )
        return FALSE;
    if( !fb_hIntlGetInfo( &Info ) )
        return FALSE;

    switch( Info.date_format ) {
    case 0:
        strcpy( buffer, "MM/dd/yyyy" );
        break;
    case 1:
        strcpy( buffer, "dd/MM/yyyy" );
        break;
    case 2:
        strcpy( buffer, "yyyy/MM/dd" );
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

/* end of drivers/dos/drv_intl_getdateformat.c */
