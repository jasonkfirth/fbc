/*
 *  libfb - FreeBASIC's runtime library
 *	Copyright (C) 2004-2007 The FreeBASIC development team.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  As a special exception, the copyright holders of this library give
 *  you permission to link this library with independent modules to
 *  produce an executable, regardless of the license terms of these
 *  independent modules, and to copy and distribute the resulting
 *  executable under terms of your choice, provided that you also meet,
 *  for each linked independent module, the terms and conditions of the
 *  license of that module. An independent module is a module which is
 *  not derived from or based on this library. If you modify this library,
 *  you may extend this exception to your version of the library, but
 *  you are not obligated to do so. If you do not wish to do so, delete
 *  this exception statement from your version.
 */

/*
 * io_mouse.c -- mouse functions for Windows console mode apps
 *
 * chng: jun/2005 written [lillo]
 *
 */

#include "fb.h"

static int inited = -1;
static int last_x = 0, last_y = 0, last_z = 0, last_buttons = 0;

static
void ProcessMouseEvent(const MOUSE_EVENT_RECORD *pEvent)
{
    if( pEvent->dwEventFlags == MOUSE_WHEELED ) {
        last_z += ( ( pEvent->dwButtonState & 0xFF000000 ) ? -1 : 1 );
    }
    else {
        last_x = pEvent->dwMousePosition.X;
        last_y = pEvent->dwMousePosition.Y;
        last_buttons = pEvent->dwButtonState & 0x7;
    }
}

/*:::::*/
int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
#if 0
	INPUT_RECORD ir;
    DWORD dwRead;
#endif

  DWORD dwMode;

	if( inited == -1 ) {
		inited = GetSystemMetrics( SM_CMOUSEBUTTONS );
		if( inited ) {
			GetConsoleMode( __fb_in_handle, &dwMode );
			dwMode |= ENABLE_MOUSE_INPUT;
			SetConsoleMode( __fb_in_handle, dwMode );
#if 1
            __fb_con.mouseEventHook = ProcessMouseEvent;
#endif
            last_x = last_y = 1;
            fb_hConvertToConsole( &last_x, &last_y, NULL, NULL );
		}
	}
	if( inited == 0 ) {
		*x = *y = *z = *buttons = -1;
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}
	if( inited > 0) {
		GetConsoleMode( __fb_in_handle, &dwMode );
		if( !(dwMode & ENABLE_MOUSE_INPUT) )
		{
			dwMode |= ENABLE_MOUSE_INPUT;
			SetConsoleMode( __fb_in_handle, dwMode );
		}
	}

#if 0
	if( PeekConsoleInput( __fb_in_handle, &ir, 1, &dwRead ) ) {
		if( dwRead > 0 ) {
			ReadConsoleInput( __fb_in_handle, &ir, 1, &dwRead );
            if( ir.EventType == MOUSE_EVENT ) {
                ProcessMouseEvent( &ir.Event.MouseEvent );
			}
		}
    }
#else
    fb_ConsoleProcessEvents  ( );
#endif

	*x = last_x - 1;
	*y = last_y - 1;
	*z = last_z;
    *buttons = last_buttons;
    *clip = 0;

    fb_hConvertFromConsole( x, y, NULL, NULL );

	return FB_RTERROR_OK;
}


/*:::::*/
int fb_ConsoleSetMouse( int x, int y, int cursor, int clip )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}