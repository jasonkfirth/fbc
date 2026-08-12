/*
 * Project: FreeBASIC RISC OS narrow-to-wide conversion
 * ----------------------------------------------------
 *
 * File: strw_convfrom_str.c
 *
 * Purpose:
 *
 *     Convert narrow strings without UnixLib length-probe assumptions.
 *
 * Responsibilities:
 *
 *     - allocate FreeBASIC wide strings from narrow input
 *     - provide deterministic ASCII fallback conversion
 *     - avoid a null-destination mbstowcs length probe
 *
 * This file intentionally does NOT contain:
 *
 *     - locale configuration
 *     - UTF file encoding
 *     - wide-to-narrow conversion
 */
/* ascii to unicode string convertion function */

#include "../fb.h"

static ssize_t fb_wstr_ConvFromA_nomultibyte(FB_WCHAR *dst, ssize_t dst_chars, const char *src)
{
	/* mbstowcs() must have failed; translate at least ASCII chars
	   and write out '?' for the others */
	FB_WCHAR *origdst = dst;
	FB_WCHAR *dstlimit = dst + dst_chars;
	while (dst < dstlimit) {
		unsigned char c = *src++;
		if (c == 0)
			break;
		if (c > 127)
			c = '?';
		*dst++ = c;
	}
	*dst = _LC('\0');
	return dst - origdst;
}

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

	/* plus the null-term (note: "n" in chars, not bytes!) */
	ssize_t chars = mbstowcs(dst, src, dst_chars + 1);

	/* worked? */
	if (chars >= 0) {
		/* a null terminator won't be added if there was not
		   enough space, so do it manually (this will cut off the last
		   char, but what can you do) */
		if (chars == (dst_chars + 1)) {
			dst[dst_chars] = _LC('\0');
			return dst_chars - 1;
		}
		return chars;
	}

	/* mbstowcs() failed?; translate at least ASCII chars
	** and write out '?' for the others
	*/
	return fb_wstr_ConvFromA_nomultibyte( dst, dst_chars, src );
}

FBCALL FB_WCHAR *fb_StrToWstr( const char *src )
{
	FB_WCHAR *dst;
	ssize_t chars;

	if( src == NULL )
		return NULL;

	/* UnixLib does not compute a length when the destination is NULL. */
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

/* end of strw_convfrom_str.c */
