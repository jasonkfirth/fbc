/*
 * Project: FreeBASIC runtime library
 * File: legacy/legacy_str_ucase.c
 * Purpose: Provide legacy str ucase runtime support.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_UCASE( FBSTRING *src )
{
	return fb_StrUcase2( src, 0 );
}

/* end of legacy/legacy_str_ucase.c */
