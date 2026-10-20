/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_width.c
 * Purpose: console width() function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleWidth( int cols, int rows )
{
	return 0;
}

/* end of drivers/xbox/io_width.c */
