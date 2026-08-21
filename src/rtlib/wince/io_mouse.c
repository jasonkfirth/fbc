/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_mouse.c

    Purpose:

        Expose pointer state to Windows CE console-mode programs.

    Responsibilities:

        - map the system pointer from screen pixels to logical text cells
        - report primary, secondary, and middle button state
        - map logical SETMOUSE positions back to screen pixels

    This file intentionally does NOT contain:

        - desktop console mouse-event records
        - graphics-window relative coordinates
        - wheel accumulation unavailable from polling APIs

    Windows CE pointer model:

        Pen and mouse devices share the system cursor APIs.  Console programs
        have no HWND or console input queue, so coordinates are scaled against
        the physical screen and the process-local text grid.
*/

#include "../fb.h"
#include "fb_private_console.h"

static int hButtonDown( int virtual_key )
{
	return (GetAsyncKeyState( virtual_key ) & 0x8000) != 0;
}

int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
	POINT point;
	int screen_width;
	int screen_height;
	int state;

	if( !GetCursorPos( &point ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	screen_width = GetSystemMetrics( SM_CXSCREEN );
	screen_height = GetSystemMetrics( SM_CYSCREEN );
	if( (screen_width <= 0) || (screen_height <= 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( x != NULL )
		*x = (int)(((long)point.x * __fb_con.columns) / screen_width);
	if( y != NULL )
		*y = (int)(((long)point.y * __fb_con.rows) / screen_height);
	if( z != NULL )
		*z = 0;

	state = 0;
	if( hButtonDown( VK_LBUTTON ) )
		state |= 1;
	if( hButtonDown( VK_RBUTTON ) )
		state |= 2;
	if( hButtonDown( VK_MBUTTON ) )
		state |= 4;
	if( buttons != NULL )
		*buttons = state;
	if( clip != NULL )
		*clip = 0;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_ConsoleSetMouse( int x, int y, int cursor, int clip )
{
	int pixel_x;
	int pixel_y;

	(void)clip;
	if( (x >= 0) && (y >= 0) &&
	    (__fb_con.columns > 0) && (__fb_con.rows > 0) ) {
		pixel_x = (x * GetSystemMetrics( SM_CXSCREEN )) / __fb_con.columns;
		pixel_y = (y * GetSystemMetrics( SM_CYSCREEN )) / __fb_con.rows;
		if( !SetCursorPos( pixel_x, pixel_y ) )
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( cursor >= 0 )
		(void)ShowCursor( cursor != 0 );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of wince/io_mouse.c */
