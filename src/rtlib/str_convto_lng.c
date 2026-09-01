/*
    FreeBASIC Runtime Library
    -------------------------

    File: str_convto_lng.c

    Purpose:

        Convert signed and unsigned 64-bit integers to decimal strings.

    Responsibilities:

        - allocate temporary FreeBASIC string results
        - perform bounded decimal formatting
        - record the actual string length after conversion

    This file intentionally does NOT contain:

        - QuickBASIC leading-space behavior
        - floating-point formatting
        - arbitrary-radix conversion
*/

#include "fb.h"


/* ------------------------------------------------------------------------- */
/* Decimal conversion                                                        */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_LongintToStr ( long long num )
{
	FBSTRING 	*dst;
	int written;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		written = snprintf( dst->data, (size_t)dst->size + 1, "%lld", num );
		if( (written < 0) || (written > dst->size) ) {
			fb_hStrDelTemp( dst );
			return &__fb_ctx.null_desc;
		}

		fb_hStrSetLength( dst, (size_t)written );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/*:::::*/
FBCALL FBSTRING *fb_ULongintToStr ( unsigned long long num )
{
	FBSTRING 	*dst;
	int written;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		written = snprintf( dst->data, (size_t)dst->size + 1, "%llu", num );
		if( (written < 0) || (written > dst->size) ) {
			fb_hStrDelTemp( dst );
			return &__fb_ctx.null_desc;
		}

		fb_hStrSetLength( dst, (size_t)written );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/* end of str_convto_lng.c */
