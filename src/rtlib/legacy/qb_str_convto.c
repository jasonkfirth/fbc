/*
 * Project: FreeBASIC runtime library
 * File: legacy/qb_str_convto.c
 * Purpose: Provide qb str convto runtime support.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/* QB compatible str$ routines for int, uint
 *
 * the result string's len is being "faked" to appear as if it were shorter
 * than the one that has to be allocated to fit _itoa and _gvct buffers.
 */

#include "fb.h"


/*:::::*/
FBCALL FBSTRING *fb_IntToStrQB ( int num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( int ) * 3 );
	if( dst != NULL )
	{
		/* convert */
		sprintf( dst->data, "% d", num );
			fb_hStrSetLength( dst, strlen( dst->data ) );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/*:::::*/
FBCALL FBSTRING *fb_UIntToStrQB ( unsigned int num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, sizeof( int ) * 3 );
	if( dst != NULL )
	{
		/* convert */
		sprintf( dst->data, " %u", num );
		fb_hStrSetLength( dst, strlen( dst->data ) );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/* end of legacy/qb_str_convto.c */
