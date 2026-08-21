/*
    FreeBASIC runtime library
    -------------------------

    File: wince/strw_convto_str.c

    Purpose:

        Convert UTF-16 wide strings to UTF-8 narrow strings on Windows CE.

    Responsibilities:

        * convert FB_WCHAR input without relying on missing CE code pages
        * guarantee a NUL-terminated output within the caller-provided limit
        * avoid truncating inside a multibyte UTF-8 sequence

    This file intentionally does not contain:

        * Windows ANSI code-page conversion
        * file encoding policy
*/

#include "../fb.h"

static ssize_t hFallbackConvToA( char *dst, ssize_t dst_chars, const FB_WCHAR *src )
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

static ssize_t hFitUTF8Prefix( const char *src, ssize_t limit )
{
	ssize_t used = 0;

	while( used < limit )
	{
		unsigned char c = (unsigned char) src[used];
		ssize_t chlen;
		ssize_t i;

		if( c < 0x80 )
			chlen = 1;
		else if( (c & 0xE0) == 0xC0 )
			chlen = 2;
		else if( (c & 0xF0) == 0xE0 )
			chlen = 3;
		else if( (c & 0xF8) == 0xF0 )
			chlen = 4;
		else
			return used;

		if( used + chlen > limit )
			break;

		for( i = 1; i < chlen; i++ )
		{
			if( (unsigned char) src[used + i] < 0x80 ||
			    (unsigned char) src[used + i] > 0xBF )
				return used;
		}

		used += chlen;
	}

	return used;
}

#if defined( DISABLE_WCHAR )

ssize_t fb_wstr_ConvToA( char *dst, ssize_t dst_chars, const FB_WCHAR *src )
{
	ssize_t chars = strlen( src );
	if( chars > dst_chars )
		chars = dst_chars;

	memcpy( dst, src, chars + 1 );
	return chars;
}

FBCALL FBSTRING *fb_WstrToStr( const FB_WCHAR *src )
{
	if( src == NULL )
		return &__fb_ctx.null_desc;

	ssize_t chars = fb_wstr_Len( src );
	FBSTRING *dst;

	if( chars == 0 )
		return &__fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &__fb_ctx.null_desc;

	fb_wstr_ConvToA( dst->data, chars, src );
	return dst;
}

#else

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least dst_chars+1 bytes.
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvToA( char *dst, ssize_t dst_chars, const FB_WCHAR *src )
{
	char *origdst = dst;
	char *utf8;
	ssize_t bytes;
	ssize_t chars;

	if( src == NULL )
	{
		*dst = '\0';
		return 0;
	}

	if( dst_chars <= 0 )
	{
		*dst = '\0';
		return 0;
	}

	chars = fb_wstr_Len( src );
	if( chars == 0 )
	{
		*dst = '\0';
		return 0;
	}

	utf8 = fb_WCharToUTF( FB_FILE_ENCOD_UTF8, src, chars, NULL, &bytes );
	if( utf8 == NULL )
		return hFallbackConvToA( origdst, dst_chars, src );

	if( bytes >= dst_chars )
	{
		ssize_t copy_len = hFitUTF8Prefix( utf8, dst_chars );

		if( copy_len == 0 )
		{
			*dst = '?';
			dst[1] = '\0';
			free( utf8 );
			return 1;
		}

		memcpy( dst, utf8, copy_len );
		origdst[copy_len] = '\0';
		free( utf8 );
		return copy_len;
	}

	memcpy( dst, utf8, bytes );
	dst[bytes] = '\0';
	free( utf8 );
	return bytes;
}

FBCALL FBSTRING *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING *dst;
	ssize_t chars;
	ssize_t bytes;

	if( src == NULL )
		return &__fb_ctx.null_desc;

	chars = fb_wstr_Len( src );
	if( chars == 0 )
		return &__fb_ctx.null_desc;

	/* Worst case is up to 4 UTF-8 bytes per wide character. */
	dst = fb_hStrAllocTemp( NULL, chars * 4 );
	if( dst == NULL )
		return &__fb_ctx.null_desc;

	bytes = fb_wstr_ConvToA( dst->data, chars * 4, src );
	fb_hStrSetLength( dst, bytes );

	if( bytes == 0 )
	{
		fb_hStrDelTemp( dst );
		return &__fb_ctx.null_desc;
	}

	return dst;
}

#endif

/* end of wince/strw_convto_str.c */
