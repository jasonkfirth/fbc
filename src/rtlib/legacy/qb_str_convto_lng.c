/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: legacy/qb_str_convto_lng.c

    Purpose:

        Format 64-bit integers using QuickBASIC STR$ spacing rules.

    Responsibilities:

        - allocate temporary FreeBASIC string results
        - prefix non-negative values with one space
        - perform bounded decimal formatting

    This file intentionally does NOT contain: - normal FreeBASIC STR$ formatting
        - floating-point formatting
        - arbitrary-radix conversion
*/

#include "fb.h"


/* ------------------------------------------------------------------------- */
/* QuickBASIC decimal conversion                                             */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING_ACCESS *fb_LongintToStrQB ( long long num )
{
	FBSTRING_ACCESS 	*dst;
	int written;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		written = snprintf( dst->data, (size_t)dst->size + 1,
		                    (num >= 0) ? " %lld" : "%lld", num );
		if( (written < 0) || (written > dst->size) ) {
			fb_hStrDelTemp( dst );
			return &fb_ctx.null_desc;
		}
		fb_hStrSetLength( dst, (size_t)written );
	}
	else
		dst = &fb_ctx.null_desc;

	return dst;
}

/*:::::*/
FBCALL FBSTRING_ACCESS *fb_ULongintToStrQB ( unsigned long long num )
{
	FBSTRING_ACCESS 	*dst;
	int written;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		written = snprintf( dst->data, (size_t)dst->size + 1,
		                    " %llu", num );
		if( (written < 0) || (written > dst->size) ) {
			fb_hStrDelTemp( dst );
			return &fb_ctx.null_desc;
		}
		fb_hStrSetLength( dst, (size_t)written );
	}
	else
		dst = &fb_ctx.null_desc;

	return dst;
}

/* end of legacy/qb_str_convto_lng.c */
