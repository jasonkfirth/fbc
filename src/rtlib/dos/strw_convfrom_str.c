/*
    DOS narrow-to-wide string implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* ascii to unicode string convertion function */

#include "fb.h"

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least (dst_chars + 1) * sizeof(FB_WCHAR).
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvFromA(FB_WCHAR *dst, ssize_t dst_chars, const char *src)
{
	if (src == NULL) {
		*dst = _LC('\0');
		return 0;
	}

	ssize_t chars = strlen(src);
	if (chars > dst_chars) {
		chars = dst_chars;
	}
	memcpy(dst, src, chars + 1);

	/* ensure that the null terminator is written, string may have been truncated */
	dst[chars] = '\0';
	return chars;
}

FBCALL FB_WCHAR *fb_StrToWstr( const char *src )
{
	FB_WCHAR *dst;
	ssize_t chars;

	if( src == NULL )
		return NULL;

	/* on DOS, mbstowcs() simply calls memcpy() and won't compute
	length  see fb_unicode.h */
	chars = strlen( src );
	if( chars == 0 )
		return NULL;

	dst = fb_wstr_AllocTemp( chars );
	if( dst == NULL ) {
		return NULL;
	}

	fb_wstr_ConvFromA( dst, chars, src );

	return dst;
}
