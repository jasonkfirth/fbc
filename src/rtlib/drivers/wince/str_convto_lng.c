/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/str_convto_lng.c

    Purpose:

        Convert signed and unsigned 64-bit integers to decimal FreeBASIC
        strings on Windows CE.

    Responsibilities:

        - allocate temporary result strings
        - use bounded CeGCC long-long formatting
        - reject formatting failures without returning partial text

    This file intentionally does NOT contain: - QuickBASIC leading-space behavior
        - arbitrary-radix formatting
        - desktop-only _i64toa() calls
*/

#include "../../fb.h"

static FBSTRING_ACCESS *fb_hWinceLongintResult( void )
{
	return fb_hStrAllocTemp( NULL, sizeof( unsigned long long ) * 3 );
}

FBCALL FBSTRING_ACCESS *fb_LongintToStr( long long num )
{
	FBSTRING_ACCESS *destination = fb_hWinceLongintResult();
	int written;

	if( destination == NULL )
		return &fb_ctx.null_desc;

	written = snprintf(
		destination->data,
		(size_t)destination->size + 1,
		"%lld",
		num );
	if( written < 0 || written > destination->size ) {
		fb_hStrDelTemp( destination );
		return &fb_ctx.null_desc;
	}

	fb_hStrSetLength( destination, (size_t)written );
	return destination;
}

FBCALL FBSTRING_ACCESS *fb_ULongintToStr( unsigned long long num )
{
	FBSTRING_ACCESS *destination = fb_hWinceLongintResult();
	int written;

	if( destination == NULL )
		return &fb_ctx.null_desc;

	written = snprintf(
		destination->data,
		(size_t)destination->size + 1,
		"%llu",
		num );
	if( written < 0 || written > destination->size ) {
		fb_hStrDelTemp( destination );
		return &fb_ctx.null_desc;
	}

	fb_hStrSetLength( destination, (size_t)written );
	return destination;
}

/* end of drivers/wince/str_convto_lng.c */
