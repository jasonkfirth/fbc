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
 * io_gethnd - console handle getter
 *
 * chng: dec/2005 written [v1ctor]
 *
 */

#include "fb_con.h"

static HANDLE in_handle, out_handle;
static int is_init = FALSE;

/*:::::*/
HANDLE fb_hConsoleGetHandle( int is_input )
{
	if( is_init == FALSE )
	{
		is_init = TRUE;

		in_handle = GetStdHandle( STD_INPUT_HANDLE );
		out_handle = GetStdHandle( STD_OUTPUT_HANDLE );

    	if( in_handle != NULL )
	    {
    	    /* Initialize console mode to enable processed input */
        	DWORD dwMode;
        	if( GetConsoleMode( in_handle, &dwMode ) )
        	{
            	dwMode |= ENABLE_PROCESSED_INPUT;
            	SetConsoleMode( in_handle, dwMode );
        	}
    	}
    }

	return (is_input? in_handle : out_handle);
}

void fb_hConsoleResetHandle( int is_input )
{
	if( is_input )
	{
		freopen( "CONIN$", "r", stdin );
		is_init = FALSE;
	}
	else 
	{
		freopen( "CONOUT$", "w", stdout );
		is_init = FALSE;
	}
}