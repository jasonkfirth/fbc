/*
    FreeBASIC runtime library
    -------------------------

    File: strw_convfrom_str.c

    Purpose:

        Convert narrow strings to wide strings when building for Solaris/illumos.

    Responsibilities:

        * convert UTF-8 source text to FB_WCHAR using the runtime converter
        * preserve existing DISABLE_WCHAR behavior and truncation rules

    This file intentionally does NOT contain:

        * platform-specific multibyte conversion using mbstowcs() locale tables
*/

#include "../fb.h"

#if !defined( HOST_DOS ) && !defined( HOST_ANDROID ) && !defined( HOST_JS )

static ssize_t fb_wstr_ConvFromA_nomultibyte( FB_WCHAR *dst, ssize_t dst_chars, const char *src )
{
	/* mbstowcs() and wcstombs() must have failed; translate at least ASCII
	   chars and write out '?' for the others. */
	FB_WCHAR *origdst = dst;
	FB_WCHAR *dstlimit = dst + dst_chars;
	while( dst < dstlimit )
	{
		unsigned char c = *src++;
		if( c == 0 )
			break;
		if( c > 127 )
			c = '?';
		*dst++ = c;
	}
	*dst = _LC('\0');
	return dst - origdst;
}

#endif

#if defined( DISABLE_WCHAR )

ssize_t fb_wstr_ConvFromA( FB_WCHAR *dst, ssize_t dst_chars, const char *src )
{
	ssize_t chars = strlen( src );
	if( chars > dst_chars )
		chars = dst_chars;
	memcpy( dst, src, chars + 1 );

	/* ensure that the null terminator is written, string may have been truncated */
	dst[chars] = '\0';
	return chars;
}

#else

ssize_t fb_wstr_ConvFromA( FB_WCHAR *dst, ssize_t dst_chars, const char *src )
{
	ssize_t chars;

	if( src == NULL )
	{
		*dst = _LC('\0');
		return 0;
	}

	chars = dst_chars + 1;
	fb_UTFToWChar( FB_FILE_ENCOD_UTF8, src, dst, &chars );

	if( chars > dst_chars )
	{
		dst[dst_chars] = _LC('\0');
		return dst_chars;
	}
	return chars;
}

#endif

FBCALL FB_WCHAR *fb_StrToWstr( const char *src )
{
	FB_WCHAR *dst;
	ssize_t chars = 0;

	if( src == NULL )
		return NULL;

#if defined( DISABLE_WCHAR )
	chars = strlen( src );
	if( chars == 0 )
		return NULL;
	dst = fb_wstr_AllocTemp( chars );
	if( dst == NULL )
		return NULL;
	fb_wstr_ConvFromA_nomultibyte( dst, chars, src );
	return dst;
#else
	dst = fb_UTFToWChar( FB_FILE_ENCOD_UTF8, src, NULL, &chars );
	if( chars == 0 )
	{
		fb_wstr_Del( dst );
		return NULL;
	}
	return dst;
#endif
}

/* end of strw_convfrom_str.c */
