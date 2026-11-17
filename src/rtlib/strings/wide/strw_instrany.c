/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_instrany.c
 * Purpose: instranyw function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL ssize_t fb_WstrInstrAny( ssize_t start, const FB_WCHAR *src, const FB_WCHAR *patt )
{
	ssize_t r = 0;
	size_t offset;

	if( (src != NULL) && (patt != NULL) )
	{
		ssize_t size_src = fb_wstr_Len( src );

		if( (start > 0) && (start <= size_src) )
		{
			/* wcscspn returns size_t; bound it before converting back to ssize_t. */
			offset = fb_wstr_InstrAny( &src[start-1], patt );
			if( offset <= (size_t)(size_src - start) )
				r = start + (ssize_t)offset;
		}
	}

	return r;
}

/* end of strings/wide/strw_instrany.c */
