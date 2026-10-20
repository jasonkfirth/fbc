/*
 * Project: FreeBASIC runtime library
 * File: strings/str_chr.c
 * Purpose: chr$ routine.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBSTRING *fb_CHR ( int args, ... )
{
	FBSTRING 	*dst;
	va_list 	ap;
	unsigned int num;
	int i;

	if( args <= 0 )
		return &__fb_ctx.null_desc;

	va_start( ap, args );

	/* alloc temp string */
    dst = fb_hStrAllocTemp( NULL, args );
	if( dst != NULL )
	{
		/* convert */
		for( i = 0; i < args; i++ )
		{
			num = va_arg( ap, unsigned int );
			dst->data[i] = (unsigned char)num;
		}
		dst->data[args] = '\0';
	}
	else
		dst = &__fb_ctx.null_desc;

	va_end( ap );

	return dst;
}

/* end of strings/str_chr.c */
