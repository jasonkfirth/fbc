/*
 * Project: FreeBASIC runtime library
 * File: strings/str_tempdescz.c
 * Purpose: temp string descriptor allocation for zstring's.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <string.h>

/* Temp descriptors borrow input bytes, but their legacy ABI stores char *. */
typedef char FB_STRING_DATA_POINTER_WIDTH[
	(sizeof(char *) == sizeof(const char *)) ? 1 : -1];

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescZEx( const char *str, ssize_t len )
{
	FBSTRING_ACCESS *dsc;

	FB_STRLOCK();

 	/* alloc a temporary descriptor */
 	dsc = fb_hStrAllocTempDesc( );

    FB_STRUNLOCK();

    if( dsc == NULL )
        return &fb_ctx.null_desc;

    memcpy((void *)&dsc->data, (const void *)&str, sizeof(dsc->data));
	dsc->len = len;
	dsc->size = len;

	return dsc;
}

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescZ( const char *str )
{
	size_t len = 0;

	/* find the true size */
	if( str != NULL )
		len = strlen( str );
	if( len > (size_t)FB_STRSIZEMSK )
		return &fb_ctx.null_desc;

	return fb_StrAllocTempDescZEx( str, (ssize_t)len );
}

/* end of strings/str_tempdescz.c */
