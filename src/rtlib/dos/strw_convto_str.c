/*
    DOS wide-to-narrow string implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* unicode to ascii string convertion function */

#include "../fb.h"

/* dst_chars == room in dst buffer without null terminator. Thus, the dst buffer
   must be at least dst_chars+1 bytes.
   src must be null-terminated.
   result = number of chars written, excluding null terminator that is always written */
ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars, const FB_WCHAR *src)
{
	if (src == NULL) {
		*dst = '\0';
		return 0;
	}

	ssize_t chars = strlen(src);
	if (chars > dst_chars)
		chars = dst_chars;

	memcpy(dst, src, chars + 1);
	return chars;
}

FBCALL FBSTRING *fb_WstrToStr( const FB_WCHAR *src )
{
	FBSTRING *dst;
	ssize_t chars;

	if( src == NULL )
		return &__fb_ctx.null_desc;

	/* on DOS, wcstombs() simply calls memcpy() and won't compute
	   length, see fb_unicode.h */
	chars = fb_wstr_Len( src );
	if( chars == 0 )
		return &__fb_ctx.null_desc;

	dst = fb_hStrAllocTemp( NULL, chars );
	if( dst == NULL )
		return &__fb_ctx.null_desc;

	fb_wstr_ConvToA( dst->data, chars, src );

	return dst;
}
