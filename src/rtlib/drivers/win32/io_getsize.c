/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_getsize.c
 * Purpose: Provide io getsize runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
    int nrows, ncols;

    fb_InitConsoleWindow();

    if( FB_CONSOLE_WINDOW_EMPTY() )
    {
        ncols = FB_SCRN_DEFAULT_WIDTH;
        nrows = FB_SCRN_DEFAULT_HEIGHT;
    }
    else
    {
        fb_hConsoleGetWindow( NULL, NULL, &ncols, &nrows );
    }

    if( cols != NULL )
        *cols = ncols;
    if( rows != NULL )
        *rows = nrows;
}

/* end of drivers/win32/io_getsize.c */
