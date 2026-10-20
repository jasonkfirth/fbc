/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_getsize.c
 * Purpose: Provide io getsize runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <pc.h>

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
#if 0
    int toprow, botrow;
#endif

	if( cols != NULL ) {
		*cols = ScreenCols();
	}

    if( rows != NULL ) {
#if 0
		fb_ConsoleGetView( &toprow, &botrow );
        *rows = botrow - toprow + 1;
#else
        *rows = ScreenRows();
#endif
	}
}

/* end of drivers/dos/io_getsize.c */
