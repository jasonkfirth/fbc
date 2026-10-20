/*
 * Project: FreeBASIC runtime library
 * File: strings/str_misc.c
 * Purpose: misc string routines.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_SPACE( ssize_t len )
{
	FBSTRING 	*dst;

	if( len > 0 )
	{
		/* alloc temp string */
        dst = fb_hStrAllocTemp( NULL, len );
		if( dst != NULL )
		{
			/* fill it */
			memset( dst->data, 32, len );

			/* null char */
			dst->data[len] = '\0';
		}
		else
			dst = &__fb_ctx.null_desc;
    }
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/* end of strings/str_misc.c */
