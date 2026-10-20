/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_len.c
 * Purpose: wstring length function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL ssize_t fb_WstrLen( FB_WCHAR *str )
{
	if( str == NULL )
		return 0;

	return fb_wstr_Len( str );
}

/* end of strings/wide/strw_len.c */
