/*
    FreeBASIC Runtime Library
    File: str_reverse.c
    Purpose: Implement StrReverse for FreeBASIC byte strings.
    Responsibilities: Preserve embedded NULs and return an independent string.
    This file does not reverse Unicode code points or modify the input.
*/

#include "fb.h"
#include "str_vb_private.h"

FBCALL FBSTRING *fb_StrReverse( FBSTRING *src )
{
	FBSTRING *dst = &__fb_ctx.null_desc;
	ssize_t len, i;
	int error = FB_RTERROR_OK;

	FB_STRLOCK();
	len = (src && src->data) ? FB_STRSIZE( src ) : 0;
	if( len > FB_STR_VB_MAXLEN ) {
		error = FB_RTERROR_OUTOFMEM;
	} else if( len > 0 ) {
		dst = fb_hStrAllocTemp_NoLock( NULL, len );
		if( dst == NULL ) {
			dst = &__fb_ctx.null_desc;
			error = FB_RTERROR_OUTOFMEM;
		} else {
			for( i = 0; i < len; ++i )
				dst->data[i] = src->data[len - i - 1];
			dst->data[len] = '\0';
		}
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	fb_ErrorSetNum( error );
	return dst;
}

/* end of str_reverse.c */
