/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_convto_str.c
 * Purpose: unicode to ascii string convertion function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least dst_chars+1 bytes.
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars, const FB_WCHAR *src)
{
	ssize_t chars;
#if defined DISABLE_WCHAR
	size_t source_chars;
#else
	size_t max_chars, converted_chars;
#endif

	if (dst == NULL)
		return 0;

	if (src == NULL) {
		*dst = '\0';
		return 0;
	}
	if ((dst_chars < 0) || (dst_chars >= PTRDIFF_MAX))
		return 0;
	if (dst_chars == 0) {
		*dst = '\0';
		return 0;
	}

#if defined DISABLE_WCHAR
	source_chars = strlen(src);
	if (source_chars > (size_t)dst_chars)
		source_chars = (size_t)dst_chars;

	chars = (ssize_t)source_chars;
	memcpy(dst, src, source_chars);
	dst[chars] = '\0';
	return chars;
#else
	/* plus the null-term */
	max_chars = (size_t)dst_chars + 1;
	converted_chars = wcstombs(dst, src, max_chars);

	/* worked? */
	if (converted_chars != (size_t)-1) {
		chars = (ssize_t)converted_chars;
		/* a null terminator won't be added if there was not
		   enough space, so do it manually (this will cut off the last
		   char, but what can you do) */
		if (converted_chars == max_chars) {
			dst[dst_chars] = '\0';
			return (dst_chars > 0) ? dst_chars - 1 : 0;
		}
		return chars;
	}

	/* wcstombs() failed; translate at least ASCII chars
	   and write out '?' for the others */
	char *origdst = dst;
	char *dstlimit = dst + dst_chars;
	while (dst < dstlimit) {
#if defined HOST_WIN32
		UTF_16 c = *src++;
		if (c == 0)
			break;
		if (c > 127) {
			if (c >= UTF16_SUR_HIGH_START && c <= UTF16_SUR_HIGH_END)
				src++;
			c = '?';
		}
#else
		UTF_32 c = (UTF_32)*src++;
		if (c == 0)
			break;
		if (c > 127)
			c = '?';
#endif
		*dst++ = (char)c;
	}
	*dst = '\0';
	return dst - origdst;
#endif
}

FBCALL FBSTRING_ACCESS *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING_ACCESS *dst;
	ssize_t chars;
	size_t converted_chars;

	if( src == NULL )
		return &fb_ctx.null_desc;

	/* wcstombs() returns size_t and uses (size_t)-1 for conversion errors. */
	converted_chars = wcstombs( NULL, src, 0 );
	if( converted_chars == (size_t)-1 )
	{
		/*
		   If the active C locale cannot represent a WSTRING character,
		   wcstombs() reports failure instead of a byte count. The actual
		   conversion helper already has an ASCII/'?' fallback path for
		   that case, so allocate enough space for one fallback byte per
		   wide character and let fb_wstr_ConvToA() do the conversion.
		 */
		chars = fb_wstr_Len( src );
		if( chars <= 0 )
			return &fb_ctx.null_desc;
	}
	else
	{
		if( converted_chars >= (size_t)PTRDIFF_MAX )
			return &fb_ctx.null_desc;
		chars = (ssize_t)converted_chars;
	}
	if( chars == 0 )
		return &fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	fb_wstr_ConvToA( dst->data, chars, src );

	return dst;
}

/* end of strings/wide/strw_convto_str.c */
