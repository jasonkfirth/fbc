/*
 * Project: FreeBASIC runtime library
 * File: legacy/legacy_strw_ucase.c
 * Purpose: Provide legacy strw ucase runtime support.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrUcase( const FB_WCHAR *src )
{
	return fb_WstrUcase2( src, 0 );
}

/* end of legacy/legacy_strw_ucase.c */
