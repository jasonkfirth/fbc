/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_width.c
 * Purpose: console width() function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <conio.h>
#include <pc.h>

int fb_ConsoleWidth( int cols, int rows )
{
	int cur = ScreenCols() | (ScreenRows() << 16);

	if( (cols > 0) || (rows > 0) )
	{
		if( rows <= 0 )
			rows = ScreenRows();

		if( cols <= 0 )
			cols = ScreenCols();

		if (cols == 40)
			textmode(C40);
		else
			textmode(C80);

		_set_screen_lines(rows);

		/* turn off blink */
		intensevideo();
	}

	return cur;
}

/* end of drivers/dos/io_width.c */
