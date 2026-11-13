/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_multikey.c
 * Purpose: console multikey().
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleMultikey( int scancode )
{
	if( scancode < 0 ||
	    scancode >= (int)(sizeof(fb_console_context.multikey) / sizeof(fb_console_context.multikey[0])) ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return FB_FALSE;
	}

	return fb_console_context.multikey[scancode] != 0? FB_TRUE: FB_FALSE;
}

/* end of drivers/js/io_multikey.c */
