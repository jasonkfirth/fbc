/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_getxy.c
 * Purpose: Provide io getxy runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleGetRawXYEx( HANDLE hConsole, int *col, int *row )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if( GetConsoleScreenBufferInfo( hConsole, &info ) ) {
        if( col != NULL )
            *col = -1;
        if( row != NULL )
            *row = -1;
    } else {
        if( col != NULL )
            *col = info.dwCursorPosition.X;
        if( row != NULL )
            *row = info.dwCursorPosition.Y;
    }
}

void fb_ConsoleGetRawXY( int *col, int *row )
{
    fb_ConsoleGetRawXYEx( __fb_out_handle, col, row );
}

FBCALL void fb_ConsoleGetXY( int *col, int *row )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if( !GetConsoleScreenBufferInfo( __fb_out_handle, &info ) ) {
        if( col != NULL )
            *col = 0;
        if( row != NULL )
            *row = 0;
    } else {
        if( col != NULL )
            *col = info.dwCursorPosition.X;
        if( row != NULL )
            *row = info.dwCursorPosition.Y;
        fb_hConvertFromConsole( col, row, NULL, NULL );
    }
}

/* end of drivers/win32/io_getxy.c */
