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

		__fb_con.inHandle = GetStdHandle( STD_INPUT_HANDLE );
		__fb_con.outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

    	if( __fb_con.inHandle != NULL )
	    {
    	    /* Initialize console mode to enable processed input */
        	DWORD dwMode;
        	if( GetConsoleMode( __fb_con.inHandle, &dwMode ) )
        	{
            	dwMode |= ENABLE_PROCESSED_INPUT;
            	SetConsoleMode( __fb_con.inHandle, dwMode );
        	}
    	}

    	__fb_con.active = __fb_con.visible = 0;
    	__fb_con.pgHandleTb[0] = __fb_con.outHandle;
    }

	return (is_input? __fb_con.inHandle : __fb_con.pgHandleTb[__fb_con.active]);
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
