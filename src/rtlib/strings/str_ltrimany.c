/*
 * Project: FreeBASIC runtime library
 * File: strings/str_ltrimany.c
 * Purpose: ltrim$ ANY function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_LTrimAny 
	( 
		FBSTRING_ACCESS *src,
		FBSTRING_ACCESS *pattern 
	)
{
	const char *pachText = NULL;
	FBSTRING_ACCESS *dst;
	ssize_t len;

	if( src == NULL )
	{
		fb_hStrDelTemp( pattern );
		return &fb_ctx.null_desc;
	}

	FB_STRLOCK();

	len = 0;
	if( src->data != NULL )
	{
		ssize_t len_pattern = ((pattern != NULL) && (pattern->data != NULL)? FB_STRSIZE( pattern ) : 0);
		pachText = src->data;
		len = FB_STRSIZE( src );
		if( len_pattern != 0 )
		{
			while ( len != 0 )
			{
				if( FB_MEMCHR( pattern->data, *pachText, len_pattern ) == NULL )
					break;

				--len;
				++pachText;
			}
		}
	}

	if( len > 0 )
	{
		/* alloc temp string */
		dst = fb_hStrAllocTemp_NoLock( NULL, len );
		if( dst != NULL )
		{
			/* simple copy */
			fb_hStrCopy( dst->data, pachText, len );
		}
		else
			dst = &fb_ctx.null_desc;
	}
	else
		dst = &fb_ctx.null_desc;

	/* del if temp */
	fb_hStrDelTemp_NoLock( src );
	fb_hStrDelTemp_NoLock( pattern );

	FB_STRUNLOCK();

	return dst;
}

/* end of strings/str_ltrimany.c */
