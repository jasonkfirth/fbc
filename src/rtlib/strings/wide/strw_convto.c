/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_convto.c
 * Purpose: strw$ routines for int, uint.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_IntToWstr ( int num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( sizeof( int ) * 3 );
	if( dst != NULL )
	{
		/* convert */
        FB_WSTR_FROM_INT( dst, num );
	}

	return dst;
}

FBCALL FB_WCHAR *fb_UIntToWstr ( unsigned int num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( sizeof( int ) * 3 );
	if( dst != NULL )
	{
		/* convert */
        FB_WSTR_FROM_UINT( dst, num );
	}

	return dst;
}

/* end of strings/wide/strw_convto.c */
