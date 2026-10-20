/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_locate.c
 * Purpose: console LOCATE statement.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleLocate( int row, int col, int cursor )
{
	return 0;
}

/* end of drivers/xbox/io_locate.c */
