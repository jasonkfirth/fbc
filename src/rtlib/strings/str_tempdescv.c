/*
 * Project: FreeBASIC runtime library
 * File: strings/str_tempdescv.c
 * Purpose: legacy: temp string descriptor allocation for var-len strings.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_StrAllocTempDescV( FBSTRING_ACCESS *str )
{
	FBSTRING_ACCESS *dsc;

	FB_STRLOCK();

 	/* alloc a temporary descriptor */
 	dsc = fb_hStrAllocTempDesc( );

    FB_STRUNLOCK();

    if( dsc == NULL )
        return &fb_ctx.null_desc;

    dsc->data = str->data;
    dsc->len  = FB_STRSIZE( str );
    dsc->size = str->size;

    return dsc;
}

/* end of strings/str_tempdescv.c */
