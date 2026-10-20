/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/drv_intl_get.c
 * Purpose: get i18n data.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"

const char *fb_DrvIntlGet( eFbIntlIndex Index )
{
	static char buf[128];
	LCTYPE lctype;

	switch( Index ) {
	case eFIL_DateDivider: lctype = LOCALE_SDATE; break;
	case eFIL_TimeDivider: lctype = LOCALE_STIME; break;
	case eFIL_NumDecimalPoint: lctype = LOCALE_SDECIMAL; break;
	case eFIL_NumThousandsSeparator: lctype = LOCALE_STHOUSAND; break;
	default:
		return NULL;
	}

	return fb_hGetLocaleInfo( LOCALE_USER_DEFAULT, lctype,
	                          buf, sizeof(buf) - 1 ) ? buf : NULL;
}

/* end of drivers/win32/drv_intl_get.c */
