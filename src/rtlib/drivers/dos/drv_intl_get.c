/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/drv_intl_get.c
 * Purpose: get i18n data.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"

const char *fb_DrvIntlGet( eFbIntlIndex Index )
{
	static DOS_COUNTRY_INFO_GENERAL Info;
	if( !fb_hIntlGetInfo( &Info ) )
		return NULL;

	switch( Index ) {
	case eFIL_DateDivider:
		return Info.date_sep;
	case eFIL_TimeDivider:
		return Info.time_sep;
	case eFIL_NumDecimalPoint:
		return Info.decimal_sep;
	case eFIL_NumThousandsSeparator:
		return Info.thousands_sep;
	}

	return NULL;
}

/* end of drivers/dos/drv_intl_get.c */
