/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_getsize.c
 * Purpose: Provide io getsize runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
	if( cols != NULL )
		*cols = 80;
	if( rows != NULL )
		*rows = 25;
}

/* end of drivers/xbox/io_getsize.c */
