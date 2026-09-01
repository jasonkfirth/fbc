/*
    FreeBASIC Runtime Library
    -------------------------

    File: sys_winver.c

    Purpose:

        Identify the desktop Windows operating-system family at runtime.

    Responsibilities:

        - distinguish Windows 95, 98 and ME from Windows NT systems
        - provide one shared policy boundary for legacy API selection

    This file intentionally does NOT contain:

        - version-specific feature probing
        - filesystem or user-interface operations
        - compiler target selection
*/

#include "../fb.h"
#include <windows.h>

/* ------------------------------------------------------------------------- */
/* Windows family detection                                                  */
/* ------------------------------------------------------------------------- */

int fb_hWin32IsWin9x( void )
{
	DWORD version;

	/*
	   GetVersion() sets bit 31 on the Windows 95 family and clears it on the
	   NT family.  Newer Windows releases may report a compatibility version,
	   but they remain on the NT side of this boundary, which is all callers
	   need to know.
	*/
	version = GetVersion( );
	return (version & 0x80000000UL) != 0;
}

/* end of sys_winver.c */
