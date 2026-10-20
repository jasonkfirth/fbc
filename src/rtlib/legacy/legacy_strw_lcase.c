/*
 * Project: FreeBASIC runtime library
 * File: legacy/legacy_strw_lcase.c
 * Purpose: Provide legacy strw lcase runtime support.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrLcase( const FB_WCHAR *src )
{
	return fb_WstrLcase2( src, 0 );
}

/* end of legacy/legacy_strw_lcase.c */
