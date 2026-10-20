/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_space.c
 * Purpose: spacew$ function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrSpace( ssize_t chars )
{
	FB_WCHAR *dst;

	if( chars <= 0 )
		return NULL;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( chars );
	if( dst != NULL )
	{
		/* fill it */
		fb_wstr_Fill( dst, 32, chars );
	}

	return dst;
}

/* end of strings/wide/strw_space.c */
