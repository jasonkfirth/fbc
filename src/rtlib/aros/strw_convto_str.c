/*
 * Project: FreeBASIC AROS wide-to-narrow conversion
 * -------------------------------------------------
 *
 * File: strw_convto_str.c
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
 * This file intentionally does NOT contain:
 *
 *     - shared wide-string behavior
 *     - locale configuration
 *     - narrow-to-wide conversion
 */

#include "../fb.h"

/*
 * dst_chars is the usable room before the null terminator.  The caller owns
 * dst_chars + 1 bytes.  AROS wcstombs() returns the converted byte count but,
 * unlike the C contract, currently omits the terminator even when it fits.
 */
ssize_t fb_wstr_ConvToA( char *dst, ssize_t dst_chars, const FB_WCHAR *src )
{
	ssize_t chars;

	if( src == NULL || dst_chars <= 0 )
	{
		*dst = '\0';
		return 0;
	}

	chars = wcstombs( dst, src, dst_chars + 1 );
	if( chars >= 0 )
	{
		if( chars > dst_chars )
			chars = dst_chars;

		dst[chars] = '\0';
		return chars;
	}

	/* Retain useful ASCII output if the active converter rejects the input. */
	{
		char *origdst = dst;
		char *dstlimit = dst + dst_chars;

		while( dst < dstlimit )
		{
			UTF_32 c = *src++;

			if( c == 0 )
				break;
			if( c > 127 )
				c = '?';

			*dst++ = c;
		}

		*dst = '\0';
		return dst - origdst;
	}
}

FBCALL FBSTRING *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING *dst;
	ssize_t chars;
	ssize_t writtenchars;

	if( src == NULL )
		return &__fb_ctx.null_desc;

	chars = wcstombs( NULL, src, 0 );
	if( chars < 0 )
		chars = fb_wstr_Len( src );
	if( chars == 0 )
		return &__fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &__fb_ctx.null_desc;

	writtenchars = fb_wstr_ConvToA( dst->data, chars, src );
	fb_hStrSetLength( dst, writtenchars );

	return dst;
}

/* end of strw_convto_str.c */
