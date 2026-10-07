/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_getpid.c
    Purpose: Provide a C ABI entry point for Windows CE process identity.
    Responsibilities: Call the SDK accessor and preserve its 32-bit result.
    This file intentionally does NOT contain kernel-data layouts or POSIX APIs.
*/

#include "../fb.h"
#include <windows.h>

STATIC_ASSERT( sizeof(unsigned int) == 4 );

/*
 * CeGCC implements GetCurrentProcessId inline in kfuncs.h. Calling it from C
 * lets the SDK select the correct kernel-data access for ARM and MIPS; BASIC
 * declarations cannot link directly to that inline-only Windows API name.
 */
unsigned int fb_hWinCEGetProcessId( void )
{
	return GetCurrentProcessId();
}

/* end of wince/sys_getpid.c */
