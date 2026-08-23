/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: dos/io_serial_control.c

    Purpose:

        Adapt the DOS UART driver's protected helpers to the portable
        fbcom.bi backend ABI.

    Responsibilities:

        - validate the DOS OPEN COM handle
        - forward status, line, break, and purge requests by COM number

    This file intentionally does NOT contain:

        - direct UART register access
        - IRQ or ring-buffer management
        - serial stream I/O
*/

#include "../fb.h"
#include "../io_serial_private.h"

int fb_dos_SerialGetStatus( int com_num, FB_COM_STATUS *status );
int fb_dos_SerialSetLines( int com_num, unsigned int mask,
	unsigned int values );
int fb_dos_SerialSetBreak( int com_num, int enabled );
int fb_dos_SerialPurge( int com_num, unsigned int queues );

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

/* end of dos/io_serial_control.c */
