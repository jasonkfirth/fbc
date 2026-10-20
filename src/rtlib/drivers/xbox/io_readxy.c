/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_readxy.c
 * Purpose: console SCREEN() function (character/color query).
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL unsigned int fb_ConsoleReadXY( int col, int row, int colorflag )
{
	return 0;
}

/* end of drivers/xbox/io_readxy.c */
