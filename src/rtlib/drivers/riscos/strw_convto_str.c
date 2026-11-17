/*
 * Project: FreeBASIC RISC OS wide-to-narrow conversion
 * ----------------------------------------------------
 *
 * File: drivers/riscos/strw_convto_str.c
 *
 * Purpose:
 *
 *     Convert wide strings without UnixLib length-probe assumptions.
 *
 * Responsibilities:
 *
 *     - allocate FreeBASIC narrow strings from wide input
 *     - provide deterministic ASCII fallback conversion
 *     - avoid a null-destination wcstombs length probe
 *
 * This file intentionally does NOT contain: *
 *     - locale configuration
 *     - UTF file encoding
 *     - narrow-to-wide conversion
 */
/* unicode to ascii string convertion function */

#include "../../fb.h"

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least dst_chars+1 bytes.
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars, const FB_WCHAR *src)
{
	size_t converted;
	size_t destination_capacity;

	if( (dst == NULL) || (dst_chars < 0) ||
	    (dst_chars >= (ssize_t)FB_STRSIZEMSK) )
		return 0;
	if (src == NULL) {
		*dst = '\0';
		return 0;
	}

	/* plus the null-term */
	destination_capacity = (size_t)dst_chars + 1u;
	converted = wcstombs(dst, src, destination_capacity);

	/* worked? */
	if (converted != (size_t)-1) {
		/* a null terminator won't be added if there was not
		   enough space, so do it manually (this will cut off the last
		   char, but what can you do) */
		if (converted == destination_capacity) {
			dst[dst_chars] = '\0';
			return (dst_chars > 0) ? dst_chars - 1 : 0;
		}
		return (ssize_t)converted;
	}

	/* wcstombs() failed; translate at least ASCII chars
	   and write out '?' for the others */
	char *origdst = dst;
	char *dstlimit = dst + dst_chars;
	while (dst < dstlimit) {
		wchar_t source_char = *src++;
		UTF_32 c;
		if( source_char < 0 )
			c = '?';
		else
			c = (UTF_32)(unsigned int)source_char;
		if (c == 0)
			break;
		if (c > 127)
			c = '?';
		*dst++ = (char)c;
	}
	*dst = '\0';
	return dst - origdst;
}

FBCALL FBSTRING_ACCESS *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING_ACCESS *dst;
	ssize_t chars;

	if( src == NULL )
		return &fb_ctx.null_desc;

	/* UnixLib does not compute a length when the destination is NULL. */
	chars = fb_wstr_Len( src );
	if( chars <= 0 )
		return &fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	fb_wstr_ConvToA( dst->data, chars, src );

	return dst;
}

/* end of drivers/riscos/strw_convto_str.c */
