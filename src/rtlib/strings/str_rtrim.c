/*
 * Project: FreeBASIC runtime library
 * File: strings/str_rtrim.c
 * Purpose: rtrim$ function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_RTRIM( FBSTRING_ACCESS *src )
{
	FBSTRING_ACCESS *dst;
	ssize_t len;

	if( src == NULL )
		return &fb_ctx.null_desc;

   	FB_STRLOCK();

	len = 0;
	if( src->data != NULL )
	{
		len = FB_STRSIZE( src );
		if( len > 0 )
		{
			char *src_ptr = fb_hStrSkipCharRev( src->data, len, 32 );
			len = (ssize_t)(src_ptr - src->data) + 1;
		}
	}

	if( len > 0 )
	{
		/* alloc temp string */
        dst = fb_hStrAllocTemp_NoLock( NULL, len );
		if( dst != NULL )
		{
			/* simple copy */
			fb_hStrCopy( dst->data, src->data, len );
		}
		else
			dst = &fb_ctx.null_desc;
    }
	else
		dst = &fb_ctx.null_desc;

	/* del if temp */
	fb_hStrDelTemp_NoLock( src );

   	FB_STRUNLOCK();

	return dst;
}

/* end of strings/str_rtrim.c */
