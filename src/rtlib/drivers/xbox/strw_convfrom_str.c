/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/strw_convfrom_str.c
 * Purpose: ascii to unicode string convertion function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least (dst_chars + 1) * sizeof(FB_WCHAR).
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvFromA(FB_WCHAR *dst, ssize_t dst_chars, const char *src)
{
	if (src == NULL) {
		*dst = FB_WCHAR_LITERAL('\0');
		return 0;
	}

#if defined DISABLE_WCHAR
	ssize_t chars = strlen(src);
	if (chars > dst_chars) {
		chars = dst_chars;
	}
	memcpy(dst, src, chars + 1);

	/* ensure that the null terminator is written, string may have been truncated */
	dst[chars] = '\0';
	return chars;
#else
	/* Xbox program strings use UTF-8. Do not depend on the process locale. */
	ssize_t chars = dst_chars + 1;
	fb_UTFToWChar( FB_FILE_ENCOD_UTF8, src, dst, &chars );
	if( chars > dst_chars ) {
		dst[dst_chars] = FB_WCHAR_LITERAL('\0');
		return dst_chars;
	}
	return chars;
#endif
}

FBCALL FB_WCHAR *fb_StrToWstr( const char *src )
{
	FB_WCHAR *dst;
	ssize_t chars;

	if( src == NULL )
		return NULL;

	chars = 0;
	dst = fb_UTFToWChar( FB_FILE_ENCOD_UTF8, src, NULL, &chars );
	if( chars == 0 ) {
		fb_wstr_Del( dst );
		return NULL;
	}
	return dst;
}

/* end of drivers/xbox/strw_convfrom_str.c */
