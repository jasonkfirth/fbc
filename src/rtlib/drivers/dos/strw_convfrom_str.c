/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/strw_convfrom_str.c
 * Purpose: Provide strw convfrom str runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    DOS narrow-to-wide string implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* ascii to unicode string convertion function */

#include "../../fb.h"
#include <limits.h>

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least (dst_chars + 1) * sizeof(FB_WCHAR).
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvFromA(FB_WCHAR *dst, ssize_t dst_chars, const char *src)
{
	size_t source_chars, copy_chars, capacity;

	if( dst == NULL || dst_chars < 0 )
		return 0;
	if (src == NULL) {
		*dst = FB_WCHAR_LITERAL('\0');
		return 0;
	}

	capacity = (size_t)dst_chars;
	source_chars = strlen(src);
	copy_chars = source_chars;
	if( copy_chars > capacity )
		copy_chars = capacity;
	if( copy_chars > 0 )
		memcpy( dst, src, copy_chars );

	/* ensure that the null terminator is written, string may have been truncated */
	dst[copy_chars] = '\0';
	return (ssize_t)copy_chars;
}

FBCALL FB_WCHAR *fb_StrToWstr( const char *src )
{
	FB_WCHAR *dst;
	ssize_t chars;

	if( src == NULL )
		return NULL;

	/* on DOS, mbstowcs() simply calls memcpy() and won't compute
	length  see fb_unicode.h */
	{
		size_t source_chars = strlen( src );
		if( source_chars > (size_t)LONG_MAX )
			return NULL;
		chars = (ssize_t)source_chars;
	}
	if( chars == 0 )
		return NULL;

	dst = fb_wstr_AllocTemp( chars );
	if( dst == NULL ) {
		return NULL;
	}

	fb_wstr_ConvFromA( dst, chars, src );

	return dst;
}

/* end of drivers/dos/strw_convfrom_str.c */
