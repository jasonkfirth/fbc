/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_width.c
 * Purpose: console width() for Windows.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/* code based on PDCurses, Win32 port by Chris Szurgot (szurgot@itribe.net) */

#include "../../fb.h"
#include "fb_private_console.h"
#include <limits.h>

int fb_ConsoleWidth( int cols, int rows )
{
    COORD size, max;
    int cur, do_change = FALSE;
    int ncols, nrows;

    fb_InitConsoleWindow( );

    if( FB_CONSOLE_WINDOW_EMPTY() )
        return 0;

    max = GetLargestConsoleWindowSize( FB_CONSOLE_OUTPUT_HANDLE );
    fb_hConsoleGetWindow( NULL, NULL, &ncols, &nrows );

    /* COORD uses signed SHORT fields, so dimensions must fit before they
     * reach the Windows console APIs. Negative caller values still query. */
    if( (cols > SHRT_MAX) || (rows > SHRT_MAX) ||
        (ncols < 0) || (ncols > SHRT_MAX) ||
        (nrows < 0) || (nrows > SHRT_MAX) )
    {
        return 0;
    }

    if( cols > 0 )
    {
        size.X = (SHORT)cols;
        do_change = TRUE;
    }
    else
    {
        size.X = (SHORT) ncols;
    }

    if( rows > 0 )
    {
        size.Y = (SHORT)rows;
        do_change = TRUE;
    }
    else
    {
        size.Y = (SHORT) nrows;
    }

    cur = size.X | (size.Y << 16);

    if( do_change == FALSE )
    	return cur;

    SMALL_RECT rect;
    rect.Left = rect.Top = 0;
    rect.Right = (SHORT)(size.X - 1);
    if( rect.Right > max.X )
        rect.Right = max.X;

    rect.Bottom = (SHORT)(rect.Top + size.Y - 1);
    if( rect.Bottom > max.Y )
        rect.Bottom = max.Y;

    /* Ensure that the window isn't larger than the destination screen
     * buffer size */
    int do_resize = FALSE;
    SMALL_RECT rectRes;
    if( rect.Bottom < (nrows-1) )
    {
        do_resize = TRUE;
        memcpy( &rectRes, &rect, sizeof(SMALL_RECT) );
        if( rectRes.Right >= ncols )
            rectRes.Right = (SHORT)(ncols - 1);
    }
    else if( rect.Right < (ncols-1) )
    {
        do_resize = TRUE;
        memcpy( &rectRes, &rect, sizeof(SMALL_RECT) );
        if( rectRes.Bottom >= nrows )
            rectRes.Bottom = (SHORT)(nrows - 1);
    }

    if( do_resize )
    {
        int i;
        for( i = 0; i < FB_CONSOLE_MAXPAGES; i++ )
        {
            if( fb_console_context.pgHandleTb[i] != NULL )
                SetConsoleWindowInfo( fb_console_context.pgHandleTb[i], TRUE, &rectRes );
        }
    }

    /* Now set the screen buffer size and ensure that the window is
     * large enough to show the whole buffer */
    int i;
    for( i = 0; i < FB_CONSOLE_MAXPAGES; i++ )
    {
	if( fb_console_context.pgHandleTb[i] != NULL )
       	{
		SetConsoleScreenBufferSize( fb_console_context.pgHandleTb[i], size );
		SetConsoleWindowInfo( fb_console_context.pgHandleTb[i], TRUE, &rect );
       	}
    }

    /* Re-enable updating */
    fb_console_context.setByUser = FALSE;
    fb_hUpdateConsoleWindow( );
    fb_console_context.setByUser = TRUE;

    return cur;
}

/* end of drivers/win32/io_width.c */
