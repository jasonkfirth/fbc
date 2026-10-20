/*
 * Project: FreeBASIC runtime library
 * File: legacy/legacy_str_lcase.c
 * Purpose: Provide legacy str lcase runtime support.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_LCASE( FBSTRING *src )
{
	return fb_StrLcase2( src, 0 );
}

/* end of legacy/legacy_str_lcase.c */
