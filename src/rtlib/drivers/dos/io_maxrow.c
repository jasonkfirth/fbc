/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_maxrow.c
 * Purpose: Provide io maxrow runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <conio.h>

int fb_ConsoleGetMaxRow( void )
{
	struct text_info ti;
	gettextinfo( &ti );
	return ti.screenheight;
}

/* end of drivers/dos/io_maxrow.c */
