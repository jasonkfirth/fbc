/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: io_serial_control.c

    Purpose:

        Provide the fallback raw serial-control backend for targets without a
        native modem-control implementation.

    Responsibilities:

        - keep status output initialized on unsupported targets
        - return the normal unsupported-operation runtime error
        - satisfy the portable fbcom.bi runtime ABI on every target

    This file intentionally does NOT contain:

        - simulated modem state
        - direct hardware register access
        - target-specific serial APIs
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Unsupported portable serial control                                      */
/* ------------------------------------------------------------------------- */

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	(void)handle;
	(void)serial_handle;

	if( status != NULL )
		memset( status, 0, sizeof( *status ) );

	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	(void)handle;
	(void)serial_handle;
	(void)mask;
	(void)values;

	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	(void)handle;
	(void)serial_handle;
	(void)enabled;

	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	(void)handle;
	(void)serial_handle;
	(void)queues;

	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

/* end of io_serial_control.c */
