/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_instr.c
 * Purpose: instrw function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL ssize_t fb_WstrInstr( ssize_t start, const FB_WCHAR *src, const FB_WCHAR *patt )
{
	ssize_t r;
	FB_WCHAR *p;

	if( (src == NULL) || (patt == NULL) )
		return 0;

	if( (start > 0) && (start <= fb_wstr_Len( src )) && (fb_wstr_Len( patt ) != 0 ))
	{
		p = fb_wstr_Instr( &src[start-1], patt );
		if( p != NULL )
			r = fb_wstr_CalcDiff( src, p ) + 1;
		else
			r = 0;
	}
	else
		r = 0;

	return r;
}

/* end of strings/wide/strw_instr.c */
