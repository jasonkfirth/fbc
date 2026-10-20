/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_maxrow.c
 * Purpose: Provide io maxrow runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetMaxRow( void )
{
	return 100;
}

/* end of drivers/js/io_maxrow.c */
