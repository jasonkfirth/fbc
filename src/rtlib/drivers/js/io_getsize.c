/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_getsize.c
 * Purpose: Provide io getsize runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
	int res = EM_ASM_INT({
        return __fb_rtlib.console.size_get();
    }, NULL);

	if( cols != NULL ) {
		*cols = res & 0xFF;
	}

	if( rows != NULL ) {
		*rows = (res >> 8) & 0xFF;
	}
}

/* end of drivers/js/io_getsize.c */
