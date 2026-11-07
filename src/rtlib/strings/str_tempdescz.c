/*
 * Project: FreeBASIC runtime library
 * File: strings/str_tempdescz.c
 * Purpose: temp string descriptor allocation for zstring's.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescZEx( const char *str, ssize_t len )
{
	FBSTRING_ACCESS *dsc;

	FB_STRLOCK();

 	/* alloc a temporary descriptor */
 	dsc = fb_hStrAllocTempDesc( );

    FB_STRUNLOCK();

    if( dsc == NULL )
        return &fb_ctx.null_desc;

    dsc->data = (char *)str;
	dsc->len = len;
	dsc->size = len;

	return dsc;
}

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescZ( const char *str )
{
	ssize_t len;

	/* find the true size */
	if( str != NULL )
		len = strlen( str );
	else
		len = 0;

	return fb_StrAllocTempDescZEx( str, len );
}

/* end of strings/str_tempdescz.c */
