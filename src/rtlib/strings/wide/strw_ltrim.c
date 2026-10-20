/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_ltrim.c
 * Purpose: ltrimw$ function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrLTrim ( const FB_WCHAR *src )
{
	FB_WCHAR *dst;
	const FB_WCHAR *p;
	ssize_t len;

	if( src == NULL )
		return NULL;

	len = fb_wstr_Len( src );
	p = fb_wstr_SkipChar( src, len, _LC(' ') );

	len -= fb_wstr_CalcDiff( src, p );
	if( len <= 0 )
		return NULL;

	/* alloc temp string */
	dst = fb_wstr_AllocTemp( len );
	if( dst != NULL )
		fb_wstr_Copy( dst, p, len );

	return dst;
}

/* end of strings/wide/strw_ltrim.c */
