/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_pageset.c
 * Purpose: console 'screen , pg, pg' function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

HANDLE fb_hConsoleCreateBuffer( void )
{
	HANDLE hnd = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
									   		0,
									   		NULL,
									   		CONSOLE_TEXTMODE_BUFFER,
									   		NULL );
	if( hnd == NULL )
		return NULL;

	/* size must be the stdout ones */
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo( fb_console_context.outHandle, &csbi );
	SetConsoleScreenBufferSize( hnd, csbi.dwSize );

	return hnd;
}

static void hHideCursor( int pg1, int pg2 )
{
	CONSOLE_CURSOR_INFO cci;

	GetConsoleCursorInfo( fb_console_context.outHandle, &cci );
	cci.bVisible = FALSE;

	SetConsoleCursorInfo( fb_console_context.pgHandleTb[pg1], &cci );
	if( pg2 >= 0 )
		SetConsoleCursorInfo( fb_console_context.pgHandleTb[pg2], &cci );
}

int fb_ConsolePageSet ( int active, int visible )
{
	fb_hConsoleGetHandle( FALSE );

	int res = fb_console_context.active | (fb_console_context.visible << 8);

	if( active >= 0 )
	{
		if( fb_console_context.pgHandleTb[active] == NULL )
		{
            HANDLE hnd = fb_hConsoleCreateBuffer( );
            if( hnd == NULL )
            	return -1;
			else
				fb_console_context.pgHandleTb[active] = hnd;
		}

		/* if page isn't visible, hide the cursor */
		if( active != fb_console_context.visible )
		{
			hHideCursor( active, -1 );
		}

		fb_console_context.active = active;
	}

	if( visible >= 0 )
	{
		if( fb_console_context.pgHandleTb[visible] == NULL )
		{
            HANDLE hnd = fb_hConsoleCreateBuffer( );
            if( hnd == NULL )
            	return -1;
			else
				fb_console_context.pgHandleTb[visible] = hnd;
		}

		if( fb_console_context.visible != visible )
		{
            SetConsoleActiveScreenBuffer( fb_console_context.pgHandleTb[visible] );

			/* if pages aren't the same, hide the cursor */
			if( visible != fb_console_context.active )
			{
				hHideCursor( fb_console_context.active, visible );
			}

			fb_console_context.visible = visible;
		}
	}

	return res;
}

/* end of drivers/win32/io_pageset.c */
