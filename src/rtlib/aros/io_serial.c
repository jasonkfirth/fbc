/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: io_serial.c

    Purpose:

        Implement OPEN COM through the native AROS serial.device interface.

    Responsibilities:

        - own the Exec message port, extended request, and opened device unit
        - translate OPEN COM framing options into IOExtSer parameters
        - perform synchronous stream reads, writes, and input queue queries

    This file intentionally does NOT contain:

        - portable fbcom.bi modem-status translation
        - direct access to serial HIDD objects
        - asynchronous request scheduling

    Platform note:

        AROS serial devices use the Exec device model, not the Unix termios
        model used by the shared Unix backend. The runtime uses one synchronous
        IOExtSer request because the generic COM layer already serializes all
        access to an open file number. OpenDevice starts buffering received
        characters even when no read request is pending.
*/

#include <devices/serial.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <strings.h>

#include "../fb.h"
#include "../io_serial_private.h"

/* ------------------------------------------------------------------------- */
/* serial.device request helpers                                             */
/* ------------------------------------------------------------------------- */

#define FB_AROS_DEFAULT_BREAK_USEC 250000UL

static int fb_hArosSerialRequest( AROS_SERIAL_INFO *info, UWORD command )
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

static int fb_hArosSerialConfigure( AROS_SERIAL_INFO *info,
	const FB_SERIAL_OPTIONS *options )
{
	struct IOExtSer *request = info->request;

	if( (options->uiDataBits < 5) || (options->uiDataBits > 8) ||
	    (options->uiSpeed == 0) || (options->IRQNumber != 0) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	request->io_Baud = options->uiSpeed;
	request->io_ReadLen = (UBYTE)options->uiDataBits;
	request->io_WriteLen = (UBYTE)options->uiDataBits;
	request->io_StopBits = (options->StopBits == FB_SERIAL_STOP_BITS_1) ?
		1 : 2;
	request->io_BrkTime = FB_AROS_DEFAULT_BREAK_USEC;

	if( options->ReceiveBuffer != 0 )
		request->io_RBufLen = options->ReceiveBuffer;

	request->io_SerFlags &= ~(SERF_PARTY_ON | SERF_PARTY_ODD |
		SERF_7WIRE | SERF_EOFMODE);
	request->io_SerFlags |= SERF_XDISABLED;
	request->io_ExtFlags &= ~(SEXTF_MARK | SEXTF_MSPON);

	switch( options->Parity ) {
	case FB_SERIAL_PARITY_NONE:
		break;
	case FB_SERIAL_PARITY_EVEN:
		request->io_SerFlags |= SERF_PARTY_ON;
		break;
	case FB_SERIAL_PARITY_ODD:
		request->io_SerFlags |= SERF_PARTY_ON | SERF_PARTY_ODD;
		break;
	case FB_SERIAL_PARITY_SPACE:
		request->io_SerFlags |= SERF_PARTY_ON;
		request->io_ExtFlags |= SEXTF_MSPON;
		break;
	case FB_SERIAL_PARITY_MARK:
		request->io_SerFlags |= SERF_PARTY_ON;
		request->io_ExtFlags |= SEXTF_MSPON | SEXTF_MARK;
		break;
	default:
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}

	if( (options->DurationCTS != 0) && !options->SuppressRTS )
		request->io_SerFlags |= SERF_7WIRE;

	return fb_hArosSerialRequest( info, SDCMD_SETPARAMS );
}

/* ------------------------------------------------------------------------- */
/* OPEN COM stream backend                                                   */
/* ------------------------------------------------------------------------- */

int fb_SerialOpen( FB_FILE *handle, int port, FB_SERIAL_OPTIONS *options,
	const char *device, void **serial_handle )
{
	AROS_SERIAL_INFO *info;
	const char *device_name;
	int result;

	(void)handle;

	if( (options == NULL) || (serial_handle == NULL) || (port < 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	*serial_handle = NULL;
	device_name = ((port == 0) && (device != NULL) &&
		(strcasecmp( device, "COM" ) != 0)) ? device : SERIALNAME;

	info = (AROS_SERIAL_INFO *)calloc( 1, sizeof( *info ) );
	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	info->reply_port = CreateMsgPort();
	if( info->reply_port == NULL ) {
		free( info );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	info->request = (struct IOExtSer *)CreateIORequest( info->reply_port,
		sizeof( struct IOExtSer ) );
	if( info->request == NULL ) {
		DeleteMsgPort( info->reply_port );
		free( info );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	info->unit = (port > 0) ? port - 1 : 0;
	info->pOptions = options;
	info->request->io_SerFlags = SERF_XDISABLED;
	if( (options->DurationCTS != 0) && !options->SuppressRTS )
		info->request->io_SerFlags |= SERF_7WIRE;

	if( OpenDevice( device_name, (ULONG)info->unit,
	    (struct IORequest *)info->request, 0 ) != 0 ) {
		DeleteIORequest( (struct IORequest *)info->request );
		DeleteMsgPort( info->reply_port );
		free( info );
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	}

	result = fb_hArosSerialConfigure( info, options );
	if( result != FB_RTERROR_OK ) {
		CloseDevice( (struct IORequest *)info->request );
		DeleteIORequest( (struct IORequest *)info->request );
		DeleteMsgPort( info->reply_port );
		free( info );
		return fb_ErrorSetNum( result );
	}

	*serial_handle = info;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *serial_handle,
	fb_off_t *length )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;
	int result;

	(void)handle;

	if( (info == NULL) || (length == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	result = fb_hArosSerialRequest( info, SDCMD_QUERY );
	if( result != FB_RTERROR_OK ) {
		*length = 0;
		return fb_ErrorSetNum( result );
	}

	*length = (fb_off_t)info->request->IOSer.io_Actual;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialWrite( FB_FILE *handle, void *serial_handle, const void *data,
	size_t length )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;
	int result;

	(void)handle;

	if( (info == NULL) || ((data == NULL) && (length != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( length > (size_t)((ULONG)-1) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	info->request->IOSer.io_Data = (APTR)data;
	info->request->IOSer.io_Length = (ULONG)length;
	info->request->IOSer.io_Actual = 0;
	result = fb_hArosSerialRequest( info, CMD_WRITE );

	if( (result == FB_RTERROR_OK) &&
	    (info->request->IOSer.io_Actual != (ULONG)length) )
		result = FB_RTERROR_FILEIO;

	return fb_ErrorSetNum( result );
}

int fb_SerialRead( FB_FILE *handle, void *serial_handle, void *data,
	size_t *length )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;
	ULONG available;
	ULONG requested;
	int result;

	(void)handle;

	if( (info == NULL) || (length == NULL) ||
	    ((data == NULL) && (*length != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( *length > (size_t)((ULONG)-1) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	result = fb_hArosSerialRequest( info, SDCMD_QUERY );
	if( result != FB_RTERROR_OK ) {
		*length = 0;
		return fb_ErrorSetNum( result );
	}

	available = info->request->IOSer.io_Actual;
	requested = (ULONG)*length;
	if( requested > available )
		requested = available;

	if( requested == 0 ) {
		*length = 0;
		return fb_ErrorSetNum( FB_RTERROR_OK );
	}

	info->request->IOSer.io_Data = data;
	info->request->IOSer.io_Length = requested;
	info->request->IOSer.io_Actual = 0;
	result = fb_hArosSerialRequest( info, CMD_READ );
	if( result != FB_RTERROR_OK ) {
		*length = 0;
		return fb_ErrorSetNum( result );
	}

	*length = (size_t)info->request->IOSer.io_Actual;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialClose( FB_FILE *handle, void *serial_handle )
{
	AROS_SERIAL_INFO *info = (AROS_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	CloseDevice( (struct IORequest *)info->request );
	DeleteIORequest( (struct IORequest *)info->request );
	DeleteMsgPort( info->reply_port );
	free( info );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of io_serial.c */
