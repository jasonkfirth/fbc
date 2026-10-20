/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_concat.c
 * Purpose: wstring concatenation function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrConcat ( const FB_WCHAR *str1, const FB_WCHAR *str2 )
{
	FB_WCHAR *dst, *d;
	ssize_t str1_len, str2_len;

	if( str1 != NULL )
		str1_len = fb_wstr_Len( str1 );
	else
		str1_len = 0;

	if( str2 != NULL )
		str2_len = fb_wstr_Len( str2 );
	else
		str2_len = 0;

	/* NULL? */
	if( str1_len + str2_len == 0 )
		return NULL;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( str1_len + str2_len );
	if( dst == NULL )
		return NULL;

	/* do the concatenation */
    d = fb_wstr_Move( dst, str1, str1_len );
    d = fb_wstr_Move( d, str2, str2_len );
    *d = L'\0';

	return dst;
}

/* end of strings/wide/strw_concat.c */
