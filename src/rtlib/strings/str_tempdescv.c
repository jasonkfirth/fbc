/*
 * Project: FreeBASIC runtime library
 * File: strings/str_tempdescv.c
 * Purpose: legacy: temp string descriptor allocation for var-len strings.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_StrAllocTempDescV( FBSTRING *str )
{
	FBSTRING *dsc;

	FB_STRLOCK();

 	/* alloc a temporary descriptor */
 	dsc = fb_hStrAllocTempDesc( );

    FB_STRUNLOCK();

    if( dsc == NULL )
        return &__fb_ctx.null_desc;

    dsc->data = str->data;
    dsc->len  = FB_STRSIZE( str );
    dsc->size = str->size;

    return dsc;
}

/* end of strings/str_tempdescv.c */
