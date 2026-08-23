/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: win32/io_serial_control.c

    Purpose:

        Implement fbcom.bi modem controls for Win32 communications handles.

    Responsibilities:

        - translate Win32 modem status and COMSTAT values
        - switch RTS and DTR from handshake policy to explicit line control
        - assert and clear break with EscapeCommFunction
        - cancel and discard selected communication queues

    This file intentionally does NOT contain:

        - CreateFile or OPEN COM option handling
        - serial stream reads and writes
        - DOS UART register access

    Windows API interaction:

        EscapeCommFunction cannot manually drive a line while the DCB assigns
        that line to hardware handshaking. ComSetLines therefore changes only
        each selected DCB line to ENABLE or DISABLE. Unselected flow-control
        policy remains unchanged.
*/

#include "../fb.h"
#include "../io_serial_private.h"

/* ------------------------------------------------------------------------- */
/* Windows status translation                                                */
/* ------------------------------------------------------------------------- */

static unsigned int fb_hSerialWindowsErrors( DWORD errors )
{
	unsigned int result = 0;

	if( (errors & CE_BREAK) != 0 )
		result |= FB_COM_ERROR_BREAK;
	if( (errors & CE_FRAME) != 0 )
		result |= FB_COM_ERROR_FRAMING;
	if( (errors & CE_RXPARITY) != 0 )
		result |= FB_COM_ERROR_PARITY;
	if( (errors & CE_OVERRUN) != 0 )
		result |= FB_COM_ERROR_OVERRUN;
	if( (errors & CE_RXOVER) != 0 )
		result |= FB_COM_ERROR_RX_OVERFLOW;

	return result;
}

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	W32_SERIAL_INFO *info = (W32_SERIAL_INFO *)serial_handle;
	DWORD modem_status = 0;
	DWORD errors = 0;
	COMSTAT comm_status;

	(void)handle;

	if( (info == NULL) || (status == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	memset( status, 0, sizeof( *status ) );
	memset( &comm_status, 0, sizeof( comm_status ) );

	status->capabilities = FB_COM_CAP_OUTPUT_LINES | FB_COM_CAP_BREAK |
		FB_COM_CAP_PURGE_RX | FB_COM_CAP_PURGE_TX;
	status->lines = info->output_lines;

	if( GetCommModemStatus( info->hDevice, &modem_status ) ) {
		status->capabilities |= FB_COM_CAP_INPUT_LINES;

		if( (modem_status & MS_CTS_ON) != 0 )
			status->lines |= FB_COM_LINE_CTS;
		if( (modem_status & MS_DSR_ON) != 0 )
			status->lines |= FB_COM_LINE_DSR;
		if( (modem_status & MS_RLSD_ON) != 0 )
			status->lines |= FB_COM_LINE_DCD;
		if( (modem_status & MS_RING_ON) != 0 )
			status->lines |= FB_COM_LINE_RI;
	}

	/*
	    ClearCommError returns current queue lengths and consumes the pending
	    driver error word. This matches ComStatus.errors, which is a snapshot
	    of errors observed since the previous status/read operation.
	*/
	if( ClearCommError( info->hDevice, &errors, &comm_status ) ) {
		status->capabilities |= FB_COM_CAP_RX_QUEUE | FB_COM_CAP_TX_QUEUE |
			FB_COM_CAP_ERRORS;
		status->errors = fb_hSerialWindowsErrors( errors );
		status->rx_queued = (unsigned int)comm_status.cbInQue;
		status->tx_queued = (unsigned int)comm_status.cbOutQue;
	}

	return FB_RTERROR_OK;
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	W32_SERIAL_INFO *info = (W32_SERIAL_INFO *)serial_handle;
	DCB dcb;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	memset( &dcb, 0, sizeof( dcb ) );
	dcb.DCBlength = sizeof( dcb );

	if( !GetCommState( info->hDevice, &dcb ) )
		return FB_RTERROR_FILEIO;

	if( (mask & FB_COM_LINE_RTS) != 0 ) {
		dcb.fRtsControl = ((values & FB_COM_LINE_RTS) != 0) ?
			RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
	}

	if( (mask & FB_COM_LINE_DTR) != 0 ) {
		dcb.fDtrControl = ((values & FB_COM_LINE_DTR) != 0) ?
			DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
	}

	if( !SetCommState( info->hDevice, &dcb ) )
		return FB_RTERROR_FILEIO;

	info->output_lines &= ~mask;
	info->output_lines |= values & mask;

	return FB_RTERROR_OK;
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	W32_SERIAL_INFO *info = (W32_SERIAL_INFO *)serial_handle;
	DWORD operation;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	operation = enabled ? SETBREAK : CLRBREAK;

	if( !EscapeCommFunction( info->hDevice, operation ) )
		return FB_RTERROR_FILEIO;

	return FB_RTERROR_OK;
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	W32_SERIAL_INFO *info = (W32_SERIAL_INFO *)serial_handle;
	DWORD flags = 0;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( (queues & FB_COM_PURGE_RX) != 0 )
		flags |= PURGE_RXABORT | PURGE_RXCLEAR;
	if( (queues & FB_COM_PURGE_TX) != 0 )
		flags |= PURGE_TXABORT | PURGE_TXCLEAR;

	if( (flags == 0) || !PurgeComm( info->hDevice, flags ) )
		return FB_RTERROR_FILEIO;

	return FB_RTERROR_OK;
}

/* end of win32/io_serial_control.c */
