/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_concatassign.c
 * Purpose: string concat and assign (s = s + expr) function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrConcatAssign( FB_WCHAR *dst, ssize_t dst_chars, const FB_WCHAR *src )
{
	ssize_t src_len, dst_len;

	/* NULL? */
	if( (dst == NULL) || (src == NULL) )
		return dst;

	src_len = fb_wstr_Len( src );
	if( src_len == 0 )
		return dst;

	dst_len = fb_wstr_Len( dst );

	/* don't check ptr's */
	if( dst_chars > 0 )
	{
		/* less the null-term */
		--dst_chars;

		if( src_len > dst_chars - dst_len )
			src_len = dst_chars - dst_len;

		fb_wstr_Copy( &dst[dst_len], src, src_len );
	}


	return dst;
}

/* end of strings/wide/strw_concatassign.c */
