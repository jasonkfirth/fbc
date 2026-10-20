/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_left.c
 * Purpose: leftw$ function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrLeft( const FB_WCHAR *src, ssize_t chars )
{
	FB_WCHAR *dst;
	ssize_t len;

	if( src == NULL )
		return NULL;

	len = fb_wstr_Len( src );
	if( (chars <= 0) || (len == 0) )
		return NULL;

	if( chars < len )
		len = chars;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( len );
	if( dst != NULL )
		fb_wstr_Copy( dst, src, len );

	return dst;
}

/* end of strings/wide/strw_left.c */
