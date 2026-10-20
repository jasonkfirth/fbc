/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_color.c
 * Purpose: console COLOR statement.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

unsigned int fb_ConsoleColor( unsigned int fc, unsigned int bc, int flags )
{
	return 0;
}

unsigned int fb_ConsoleGetColorAtt( void )
{
	return 0;
}

/* end of drivers/xbox/io_color.c */
