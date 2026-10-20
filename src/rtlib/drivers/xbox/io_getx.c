/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_getx.c
 * Purpose: Provide io getx runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleGetX( void )
{
	return 1;
}

/* end of drivers/xbox/io_getx.c */
