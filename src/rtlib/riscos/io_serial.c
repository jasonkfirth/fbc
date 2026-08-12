/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: io_serial.c

    Purpose:

        Provide deterministic serial-device stubs for the initial port.

    Responsibilities:

        - satisfy the serial entry points used by OPEN COM
        - reject unsupported operations with the normal runtime error
        - clear caller-owned result values before returning failure

    This file intentionally does NOT contain:

        - serial device discovery
        - RISC OS module or SWI calls
        - buffering or background I/O

    Platform note:

        RISC OS serial devices do not follow the POSIX terminal-device naming
        and termios model used by the Unix backend.  A native implementation
        belongs here once its module and device ownership rules are defined.
*/

#include "../fb.h"

int fb_SerialOpen( FB_FILE *handle, int iPort, FB_SERIAL_OPTIONS *options,
	const char *pszDevice, void **ppvHandle )
{
	(void)handle;
	(void)iPort;
	(void)options;
	(void)pszDevice;

	if( ppvHandle != NULL )
		*ppvHandle = NULL;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *pvHandle, fb_off_t *pLength )
{
	(void)handle;
	(void)pvHandle;

	if( pLength != NULL )
		*pLength = 0;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialWrite( FB_FILE *handle, void *pvHandle, const void *data,
	size_t length )
{
	(void)handle;
	(void)pvHandle;
	(void)data;
	(void)length;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialRead( FB_FILE *handle, void *pvHandle, void *data,
	size_t *pLength )
{
	(void)handle;
	(void)pvHandle;
	(void)data;

	if( pLength != NULL )
		*pLength = 0;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialClose( FB_FILE *handle, void *pvHandle )
{
	(void)handle;
	(void)pvHandle;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of io_serial.c */
