/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_width.c
 * Purpose: console width() function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleWidth( int cols, int rows )
{
	return EM_ASM_INT({
		var terminal = __fb_rtlib.console;
		var oldCols = terminal.cols;
		var oldRows = terminal.rows;

		/* Terminal.resizeTo() clears its cell buffer, even at the same size.
		 * WIDTH() only queries, and omitted dimensions keep their size. */
		var newCols = $0 > 0 ? Math.max(40, $0) : oldCols;
		var newRows = $1 > 0 ? Math.max(10, $1) : oldRows;
		if (newCols !== oldCols || newRows !== oldRows)
			terminal.size_set(newCols, newRows);

		/* The BASIC WIDTH() result packs columns and rows into 16 bits each. */
		return (oldRows << 16) | (oldCols & 0xffff);
	}, cols, rows);
}

/* end of drivers/js/io_width.c */
