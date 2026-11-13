/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_gethnd.c
 * Purpose: console handle getter.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

static int is_init = FALSE;

HANDLE fb_hConsoleGetHandle( int is_input )
{
	if( is_init == FALSE )
	{
		is_init = TRUE;

		fb_console_context.inHandle = GetStdHandle( STD_INPUT_HANDLE );
		fb_console_context.outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	if( fb_console_context.inHandle != NULL )
	    {
    	    /* Initialize console mode to enable processed input */
        	DWORD dwMode;
		if( GetConsoleMode( fb_console_context.inHandle, &dwMode ) )
        	{
            	dwMode |= ENABLE_PROCESSED_INPUT;
		SetConsoleMode( fb_console_context.inHandle, dwMode );
        	}
    	}

	fb_console_context.active = fb_console_context.visible = 0;
	fb_console_context.pgHandleTb[0] = fb_console_context.outHandle;
    }

	return (is_input? fb_console_context.inHandle : fb_console_context.pgHandleTb[fb_console_context.active]);
}

void fb_hConsoleResetHandles(void)
{
	/* 
		Called by fb_FileResetEx() to cause fb_hConsoleGetHandle() 
		to reset the stored I/O handles 
	*/
	is_init = FALSE;
}

/* end of drivers/win32/io_gethnd.c */
