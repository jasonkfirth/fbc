/*
 * Project: FreeBASIC runtime library
 * File: strings/str_ucase.c
 * Purpose: ucase$ function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <ctype.h>

FBCALL FBSTRING_ACCESS *fb_StrUcase2( FBSTRING_ACCESS *src, int mode )
{
	FBSTRING_ACCESS 	*dst;
	ssize_t i, len = 0;
	int c;
	char		*s, *d;

	if( src == NULL )
		return &fb_ctx.null_desc;

	FB_STRLOCK();

	if( src->data ) {
		len = FB_STRSIZE( src );

		/* alloc temp string */
		dst = fb_hStrAllocTemp_NoLock( NULL, len );
	} else {
		dst = NULL;
	}

	if( dst ) {
		s = src->data;
		d = dst->data;

		if( mode == 1 ) {
			for( i = 0; i < len; i++ ) {
				c = FB_CHAR_TO_INT( *s++ );
				if( (c >= 97) && (c <= 122) )
					c -= 97 - 65;
				*d++ = c;
			}
		} else {
			for( i = 0; i < len; i++ ) {
				c = FB_CHAR_TO_INT( *s++ );
				if( islower( c ) )
					c = toupper( c );
				*d++ = c;
			}
		}

		/* null char */
		*d = '\0';
	} else {
		dst = &fb_ctx.null_desc;
	}

	/* del if temp */
	fb_hStrDelTemp_NoLock( src );

	FB_STRUNLOCK();

	return dst;
}

/* end of strings/str_ucase.c */
