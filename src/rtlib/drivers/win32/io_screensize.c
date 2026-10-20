/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_screensize.c
 * Purpose: size of the screen buffer.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleGetScreenSizeEx( HANDLE hConsole, int *cols, int *rows )
{
   	CONSOLE_SCREEN_BUFFER_INFO info;
    if( GetConsoleScreenBufferInfo( hConsole, &info )==0 ) {
        if( cols != NULL )
            *cols = FB_SCRN_DEFAULT_WIDTH;
        if( rows != NULL )
            *rows = FB_SCRN_DEFAULT_HEIGHT;
    } else {
        if( cols != NULL )
            *cols = info.dwSize.X;
        if( rows != NULL )
            *rows = info.dwSize.Y;
    }
}

FBCALL void fb_ConsoleGetScreenSize( int *cols, int *rows )
{
    fb_ConsoleGetScreenSizeEx( __fb_out_handle, cols, rows );
}

/* end of drivers/win32/io_screensize.c */
