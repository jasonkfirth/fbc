/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_locate.c
 * Purpose: console LOCATE statement.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleLocate( int row, int col, int cursor )
{
	if( row > 0 || col > 0 )
    {
        EM_ASM_ARGS({
            __fb_rtlib.console.pos_set($0, $1);
        }, row-1, col-1);
    }

    if( cursor != -1 )
    {
        if( cursor == 0 )
        {
            EM_ASM({
                __fb_rtlib.console.cursor_hide();
            });
        }
        else
        {
            EM_ASM({
                __fb_rtlib.console.cursor_show();
            });
        }
    }

	return (col & 0xFF) | ((row & 0xFF) << 8);
}

/* end of drivers/js/io_locate.c */
