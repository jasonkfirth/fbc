/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_getxy.c
 * Purpose: Provide io getxy runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL void fb_ConsoleGetXY( int *col, int *row )
{
	if( col != NULL )
		*col = 1;
	if( row != NULL )
		*row = 1;
}

/* end of drivers/xbox/io_getxy.c */
