/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/drv_intl_get.c
 * Purpose: get i18n data.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#ifndef DISABLE_LANGINFO
#include <langinfo.h>
#endif

const char *fb_DrvIntlGet( eFbIntlIndex Index )
{
	switch( Index ) {
	case eFIL_DateDivider:
		return "/";
	case eFIL_TimeDivider:
		return ":";
#ifdef DISABLE_LANGINFO
	case eFIL_NumDecimalPoint:
		return ".";
	case eFIL_NumThousandsSeparator:
		return ",";
#else
	case eFIL_NumDecimalPoint:
		return nl_langinfo( RADIXCHAR );
	case eFIL_NumThousandsSeparator:
		return nl_langinfo( THOUSEP );
#endif
	}
	return NULL;
}

/* end of drivers/unix/drv_intl_get.c */
