/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: drivers/dos/io_serial_control.c

    Purpose:

        Adapt the DOS UART driver's protected helpers to the portable
        fbcom.bi backend ABI.

    Responsibilities:

        - validate the DOS OPEN COM handle
        - forward status, line, break, and purge requests by COM number

    This file intentionally does NOT contain: - direct UART register access
        - IRQ or ring-buffer management
        - serial stream I/O
*/

#include "../../fb.h"
#include "../../io_serial_private.h"

/* ------------------------------------------------------------------------- */
/* Portable backend adapters                                                 */
/* ------------------------------------------------------------------------- */

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	DOS_SERIAL_INFO *info = (DOS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	return fb_dos_SerialGetStatus( info->com_num, status );
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	DOS_SERIAL_INFO *info = (DOS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	return fb_dos_SerialSetLines( info->com_num, mask, values );
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	DOS_SERIAL_INFO *info = (DOS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	return fb_dos_SerialSetBreak( info->com_num, enabled );
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	DOS_SERIAL_INFO *info = (DOS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	return fb_dos_SerialPurge( info->com_num, queues );
}

/* end of drivers/dos/io_serial_control.c */
