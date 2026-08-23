/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: aros/io_serial_control.c

    Purpose:

        Implement the fbcom.bi status and purge operations exposed by the
        native AROS serial.device interface.

    Responsibilities:

        - translate IOExtSer status bits into portable modem-line flags
        - report queued input and driver overrun/break state
        - clear the serial.device receive buffer

    This file intentionally does NOT contain:

        - opening or closing serial.device
        - direct serial HIDD access for persistent RTS/DTR output control
        - asynchronous request scheduling
*/

#include <devices/serial.h>
#include <exec/io.h>
#include <proto/exec.h>

#include "../fb.h"
#include "../io_serial_private.h"

/* ------------------------------------------------------------------------- */
/* serial.device request helper                                              */
/* ------------------------------------------------------------------------- */

static int fb_hArosSerialControlRequest( AROS_SERIAL_INFO *info,
	UWORD command )
{
	struct IOExtSer *request;

	if( (info == NULL) || (info->request == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	request = info->request;
	request->IOSer.io_Command = command;
	request->IOSer.io_Error = 0;

	if( DoIO( (struct IORequest *)request ) != 0 )
		return FB_RTERROR_FILEIO;

	if( request->IOSer.io_Error != 0 )
		return FB_RTERROR_FILEIO;

	return FB_RTERROR_OK;
}

/* ------------------------------------------------------------------------- */
/* Portable serial control backend                                           */
/* ------------------------------------------------------------------------- */

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;
	UWORD native_status;
	int result;

	(void)handle;

	if( (info == NULL) || (status == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	memset( status, 0, sizeof( *status ) );
	result = fb_hArosSerialControlRequest( info, SDCMD_QUERY );
	if( result != FB_RTERROR_OK )
		return result;

	native_status = info->request->io_Status;
	status->capabilities = FB_COM_CAP_INPUT_LINES |
		FB_COM_CAP_RX_QUEUE | FB_COM_CAP_ERRORS | FB_COM_CAP_PURGE_RX;
	status->rx_queued = (unsigned int)info->request->IOSer.io_Actual;

	/* Ring is active high. DSR, CTS, and DCD are active-low bits in the
	   serial.device status word inherited from the Amiga API. */
	if( (native_status & (1U << 2)) != 0 )
		status->lines |= FB_COM_LINE_RI;
	if( (native_status & (1U << 3)) == 0 )
		status->lines |= FB_COM_LINE_DSR;
	if( (native_status & (1U << 4)) == 0 )
		status->lines |= FB_COM_LINE_CTS;
	if( (native_status & (1U << 5)) == 0 )
		status->lines |= FB_COM_LINE_DCD;

	if( (native_status & IO_STATF_OVERRUN) != 0 )
		status->errors |= FB_COM_ERROR_RX_OVERFLOW;
	if( (native_status & IO_STATF_READBREAK) != 0 )
		status->errors |= FB_COM_ERROR_BREAK;

	return FB_RTERROR_OK;
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	(void)handle;
	(void)serial_handle;
	(void)mask;
	(void)values;

	/* serial.device reports the output state but does not define a command
	   that persistently drives individual RTS or DTR lines. */
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	(void)handle;
	(void)serial_handle;
	(void)enabled;

	/* SDCMD_BREAK sends a timed pulse; it cannot implement assert/clear. */
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( (info == NULL) || (queues != FB_COM_PURGE_RX) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	return fb_hArosSerialControlRequest( info, CMD_CLEAR );
}

/* end of aros/io_serial_control.c */
