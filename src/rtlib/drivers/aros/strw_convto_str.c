/*
 * Project: FreeBASIC AROS wide-to-narrow conversion
 * -------------------------------------------------
 *
 * File: drivers/aros/strw_convto_str.c
 *
 * Purpose:
 *
 *     Convert wide strings while compensating for the AROS wcstombs()
 *     implementation not writing a terminating null byte.
 *
 * Responsibilities:
 *
 *     - convert AROS wide strings into narrow FreeBASIC strings
 *     - guarantee a null terminator within the destination buffer
 *     - preserve deterministic ASCII fallback conversion
 *
 * This file intentionally does NOT contain: *
 *     - shared wide-string behavior
 *     - locale configuration
 *     - narrow-to-wide conversion
 */

#include "../../fb.h"

#include <limits.h>
#include <stdlib.h>

/*
 * dst_chars is the usable room before the null terminator.  The caller owns
 * dst_chars + 1 bytes.  AROS wcstombs() returns the converted byte count but,
 * unlike the C contract, currently omits the terminator even when it fits.
 */
ssize_t fb_wstr_ConvToA( char *dst, ssize_t dst_chars, const FB_WCHAR *src )
{
	ssize_t chars;
	size_t converted_chars;
	size_t destination_capacity;

	if( dst == NULL )
		return -1;
	if( src == NULL || dst_chars <= 0 )
	{
		*dst = '\0';
		return 0;
	}

	destination_capacity = (size_t)dst_chars + 1u;
	converted_chars = wcstombs( dst, src, destination_capacity );
	if( converted_chars != (size_t)-1 )
	{
		if( converted_chars > (size_t)dst_chars )
			converted_chars = (size_t)dst_chars;

		chars = (ssize_t)converted_chars;
		dst[converted_chars] = '\0';
		return chars;
	}

	/* Retain useful ASCII output if the active converter rejects the input. */
	{
		char *origdst = dst;
		char *dstlimit = dst + (size_t)dst_chars;

		while( dst < dstlimit )
		{
			UTF_32 c = *src++;

			if( c == 0 )
				break;
			if( c > 127 )
				c = '?';

			*dst++ = (char)c;
		}

		*dst = '\0';
		return dst - origdst;
	}
}

FBCALL FBSTRING_ACCESS *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING_ACCESS *dst;
	ssize_t chars;
	ssize_t writtenchars;
	size_t converted_chars;

	if( src == NULL )
		return &fb_ctx.null_desc;

	converted_chars = wcstombs( NULL, src, 0 );
	if( converted_chars == (size_t)-1 )
		chars = fb_wstr_Len( src );
	else if( converted_chars > (size_t)SSIZE_MAX )
		return &fb_ctx.null_desc;
	else
		chars = (ssize_t)converted_chars;
	if( chars <= 0 )
		return &fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	writtenchars = fb_wstr_ConvToA( dst->data, chars, src );
	if( writtenchars < 0 )
		return &fb_ctx.null_desc;
	fb_hStrSetLength( dst, (size_t)writtenchars );

	return dst;
}

/* end of drivers/aros/strw_convto_str.c */
