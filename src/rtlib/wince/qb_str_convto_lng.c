/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/qb_str_convto_lng.c

    Purpose:

        Format signed and unsigned 64-bit integers using QuickBASIC STR$
        spacing rules on Windows CE.

    Responsibilities:

        - allocate temporary FreeBASIC string results
        - prefix non-negative values with one space
        - use the long-long format supported by the CeGCC runtime

    This file intentionally does NOT contain:

        - floating-point formatting
        - arbitrary-radix conversion
        - desktop-only _i64toa() calls
*/

#include "../fb.h"

FBCALL FBSTRING *fb_LongintToStrQB( long long num )
{
	FBSTRING *destination;
	int written;

	destination = fb_hStrAllocTemp( NULL, sizeof( long long ) * 3 );
	if( destination == NULL )
		return &__fb_ctx.null_desc;

	written = snprintf(
		destination->data,
		(size_t)destination->size + 1,
		(num >= 0) ? " %lld" : "%lld",
		num );
	if( written < 0 || written > destination->size ) {
		fb_hStrDelTemp( destination );
		return &__fb_ctx.null_desc;
	}

	fb_hStrSetLength( destination, (size_t)written );
	return destination;
}

FBCALL FBSTRING *fb_ULongintToStrQB( unsigned long long num )
{
	FBSTRING *destination;
	int written;

	destination = fb_hStrAllocTemp( NULL, sizeof( unsigned long long ) * 3 );
	if( destination == NULL )
		return &__fb_ctx.null_desc;

	written = snprintf(
		destination->data,
		(size_t)destination->size + 1,
		" %llu",
		num );
	if( written < 0 || written > destination->size ) {
		fb_hStrDelTemp( destination );
		return &__fb_ctx.null_desc;
	}

	fb_hStrSetLength( destination, (size_t)written );
	return destination;
}

/* end of wince/qb_str_convto_lng.c */
