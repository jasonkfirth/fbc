/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/gpiopins.c

    Purpose:

        Provide predictable GPIO stubs for Windows CE devices without a
        portable GPIO API.

    Responsibilities:

        - export the platform-neutral GPIO runtime symbols
        - report invalid arguments and unsupported operations through the
          Windows CE last-error channel

    This file intentionally does NOT contain:

        - board-specific GPIO drivers
        - emulated GPIO state
        - Linux or NuttX device interfaces
*/

#include "../fb.h"

#include <windows.h>

/* ------------------------------------------------------------------------- */
/* Unsupported GPIO operations                                               */
/* ------------------------------------------------------------------------- */

static int fb_hWinceGpioFailure( DWORD error )
{
	SetLastError( error );
	return -1;
}

int fb_GpioPinOpen( const int pin )
{
	return fb_hWinceGpioFailure(
		(pin < 0) ? ERROR_INVALID_PARAMETER : ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinOpenDevice( const char *device )
{
	return fb_hWinceGpioFailure(
		(device == NULL || device[0] == '\0')
			? ERROR_INVALID_PARAMETER
			: ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinClose( const int handle )
{
	return fb_hWinceGpioFailure(
		(handle < 0) ? ERROR_INVALID_HANDLE : ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinRead( const int handle )
{
	return fb_hWinceGpioFailure(
		(handle < 0) ? ERROR_INVALID_HANDLE : ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinWrite( const int handle, const int value )
{
	(void)value;
	return fb_hWinceGpioFailure(
		(handle < 0) ? ERROR_INVALID_HANDLE : ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinGetType( const int handle )
{
	return fb_hWinceGpioFailure(
		(handle < 0) ? ERROR_INVALID_HANDLE : ERROR_CALL_NOT_IMPLEMENTED );
}

int fb_GpioPinSetType( const int handle, const int pin_type )
{
	(void)pin_type;
	return fb_hWinceGpioFailure(
		(handle < 0) ? ERROR_INVALID_HANDLE : ERROR_CALL_NOT_IMPLEMENTED );
}

/* end of wince/gpiopins.c */
