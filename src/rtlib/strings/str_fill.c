/*
 * Project: FreeBASIC runtime library
 * File: strings/str_fill.c
 * Purpose: string$ function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_StrFill1( ssize_t cnt, int fchar )
{
	FBSTRING_ACCESS 	*dst;

	if( cnt > 0 )
	{
		/* alloc temp string */
        dst = fb_hStrAllocTemp( NULL, cnt );
		if( dst != NULL )
		{
			/* fill it */
			memset( dst->data, fchar, cnt );
			/* null char */
			dst->data[cnt] = '\0';
		}
		else
			dst = &fb_ctx.null_desc;
	}
	else
		dst = &fb_ctx.null_desc;

	return dst;
}

FBCALL FBSTRING_ACCESS *fb_StrFill2( ssize_t cnt, FBSTRING_ACCESS *src )
{
	FBSTRING_ACCESS 	*dst;
	int fchar;

	if( (cnt > 0) && (src != NULL) && (src->data != NULL) && (FB_STRSIZE( src ) > 0) ) {
		fchar = FB_CHAR_TO_INT( src->data[0] );
		dst = fb_StrFill1( cnt, fchar );
	}
	else
		dst = &fb_ctx.null_desc;

	/* del if temp */
	fb_hStrDelTemp( src );

	return dst;
}

/* end of strings/str_fill.c */
