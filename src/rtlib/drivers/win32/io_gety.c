/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_gety.c
 * Purpose: Provide io gety runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetRawYEx( HANDLE hConsole )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if( GetConsoleScreenBufferInfo( hConsole, &info ) == 0 )
        return 0;
    return info.dwCursorPosition.Y;
}

int fb_ConsoleGetRawY( void )
{
    return fb_ConsoleGetRawYEx( __fb_out_handle );
}

int fb_ConsoleGetY( void )
{
    int y = fb_ConsoleGetRawY();
    fb_hConvertFromConsole( NULL, &y, NULL, NULL );
    return y;
}

/* end of drivers/win32/io_gety.c */
