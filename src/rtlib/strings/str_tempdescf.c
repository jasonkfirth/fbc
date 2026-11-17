/*
 * Project: FreeBASIC runtime library
 * File: strings/str_tempdescf.c
 * Purpose: temp string descriptor allocation for fixed-len strings.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <string.h>

/* Temp descriptors borrow input bytes, but their legacy ABI stores char *. */
typedef char FB_STRING_DATA_POINTER_WIDTH[
	(sizeof(char *) == sizeof(const char *)) ? 1 : -1];

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescF( const char *str, ssize_t str_size )
{
	FBSTRING_ACCESS *dsc;
	size_t len = 0;

	/* Check the only branch whose length is discovered from a C string. */
	if( (str_size <= 0) && !(str_size & FB_STRISFIXED) && (str != NULL) )
	{
		len = strlen( str );
		if( len > (size_t)FB_STRSIZEMSK )
			return &fb_ctx.null_desc;
	}

	FB_STRLOCK();

 	/* alloc a temporary descriptor */
 	dsc = fb_hStrAllocTempDesc( );

	FB_STRUNLOCK();

    if( dsc == NULL )
        return &fb_ctx.null_desc;

    memcpy((void *)&dsc->data, (const void *)&str, sizeof(dsc->data));

	/* can't use strlen() if the size is known */
	if( str_size > 0 )
	{
		/* less the null-term */
		dsc->len = str_size - 1;
	}
	else if( str_size & FB_STRISFIXED )
	{
		dsc->len = str_size & FB_STRSIZEMSK;
	}
	else
	{
		dsc->len = (ssize_t)len;
	}

	dsc->size = dsc->len;

	return dsc;
}

/* end of strings/str_tempdescf.c */
