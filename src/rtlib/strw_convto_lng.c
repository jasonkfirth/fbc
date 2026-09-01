/*
    FreeBASIC Runtime Library
    -------------------------

    File: strw_convto_lng.c

    Purpose:

        Convert signed and unsigned 64-bit integers to decimal WSTRINGs.

    Responsibilities:

        - allocate temporary wide-string results
        - use the target's native wide-integer formatting

    This file intentionally does NOT contain:

        - narrow-string conversion
        - QuickBASIC leading-space behavior
        - platform-specific CRT compatibility
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Public conversion routines                                                */
/* ------------------------------------------------------------------------- */

FBCALL FB_WCHAR *fb_LongintToWstr ( long long num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
	dst = fb_wstr_AllocTemp( sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		/* convert */
		FB_WSTR_FROM_INT64( dst, num );
	}

	return dst;
}

/*:::::*/
FBCALL FB_WCHAR *fb_ULongintToWstr ( unsigned long long num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
	dst = fb_wstr_AllocTemp( sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		/* convert */
		FB_WSTR_FROM_UINT64( dst, num );
	}

	return dst;
}

/* end of strw_convto_lng.c */
