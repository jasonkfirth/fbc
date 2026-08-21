/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: io_serial.c

    Purpose:

        Provide deterministic serial-device stubs until an AROS serial.device
        backend owns the asynchronous request and unit lifecycle.

    Responsibilities:

        - satisfy the runtime entry points used by OPEN COM
        - reject unsupported serial operations with the normal runtime error
        - initialize caller-owned output values before returning failure

    This file intentionally does NOT contain:

        - exec.library message-port or IORequest ownership
        - serial.device discovery and unit selection
        - buffering or asynchronous completion handling

    Platform note:

        AROS serial devices use the Exec device model, not the Unix termios
        model used by the shared Unix backend.  A complete implementation
        belongs in this AROS replacement file once its request ownership and
        shutdown rules are defined.
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* Unsupported serial operations                                             */
/* ------------------------------------------------------------------------- */

int fb_SerialOpen( FB_FILE *handle, int port, FB_SERIAL_OPTIONS *options,
	const char *device, void **serial_handle )
{
	(void)handle;
	(void)port;
	(void)options;
	(void)device;

	if( serial_handle != NULL )
		*serial_handle = NULL;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *serial_handle,
	fb_off_t *length )
{
	(void)handle;
	(void)serial_handle;

	if( length != NULL )
		*length = 0;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialWrite( FB_FILE *handle, void *serial_handle, const void *data,
	size_t length )
{
	(void)handle;
	(void)serial_handle;
	(void)data;
	(void)length;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialRead( FB_FILE *handle, void *serial_handle, void *data,
	size_t *length )
{
	(void)handle;
	(void)serial_handle;
	(void)data;

	if( length != NULL )
		*length = 0;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialClose( FB_FILE *handle, void *serial_handle )
{
	(void)handle;
	(void)serial_handle;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of io_serial.c */
