/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: io_serial.c

    Purpose:

        Implement OPEN COM through the RISC OS low-level serial SWIs.

    Responsibilities:

        - configure the built-in serial port from OPEN COM options
        - send and receive bytes without redirecting the process console
        - preserve and restore the machine-wide serial configuration

    This file intentionally does NOT contain:

        - add-on multi-port DeviceFS driver discovery
        - fbcom.bi modem-line controls
        - background I/O or interrupt handlers

    Platform note:

        OS_SerialOp controls the built-in serial device globally and its byte
        operations report full/empty through the carry flag. The backend uses
        _kernel_swi_c so no process input or output stream is redirected. Only
        one OPEN COM handle is permitted because baud, framing, and modem state
        are machine-wide rather than properties of a file handle.
*/

#include "../fb.h"
#include "../io_serial_private.h"

#include <kernel.h>
#include <strings.h>
#include <swis.h>

#define FB_RISCOS_SERIAL_TIMEOUT_CS 300
#define FB_RISCOS_SERIAL_READ_TIMEOUT_CS 7

static int fb_riscos_serial_open;

/* ------------------------------------------------------------------------- */
/* Native SWI helpers                                                        */
/* ------------------------------------------------------------------------- */

static int fb_hRiscosSerialOp( int reason, int value, int mask,
	_kernel_swi_regs *result, int *carry )
{
	_kernel_swi_regs registers = { { 0 } };
	_kernel_oserror *error;
	int carry_result = 0;

	registers.r[0] = reason;
	registers.r[1] = value;
	registers.r[2] = mask;
	error = _kernel_swi_c( OS_SerialOp, &registers, &registers,
		&carry_result );

	if( error != NULL )
		return FB_RTERROR_FILEIO;

	if( result != NULL )
		*result = registers;
	if( carry != NULL )
		*carry = carry_result;

	return FB_RTERROR_OK;
}

static int fb_hRiscosReadTime( int *time_cs )
{
	_kernel_swi_regs registers = { { 0 } };

	if( time_cs == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( _kernel_swi( OS_ReadMonotonicTime, &registers, &registers ) != NULL )
		return FB_RTERROR_FILEIO;

	*time_cs = registers.r[0];
	return FB_RTERROR_OK;
}

static int fb_hRiscosBaudCode( unsigned int baud )
{
	switch( baud ) {
	case 50:    return 9;
	case 75:    return 1;
	case 110:   return 10;
	case 134:   return 11;
	case 150:   return 2;
	case 300:   return 3;
	case 600:   return 12;
	case 1200:  return 4;
	case 1800:  return 13;
	case 2400:  return 5;
	case 3600:  return 14;
	case 4800:  return 6;
	case 9600:  return 7;
	case 19200: return 8;
	case 7200:  return 15;
	case 38400: return 16;
	case 57600: return 17;
	case 115200: return 18;
	default:    return -1;
	}
}

static int fb_hRiscosFormat( const FB_SERIAL_OPTIONS *options )
{
	int format;

	if( (options == NULL) || (options->uiDataBits < 5) ||
	    (options->uiDataBits > 8) )
		return -1;

	format = 8 - (int)options->uiDataBits;
	if( options->StopBits != FB_SERIAL_STOP_BITS_1 )
		format |= 1 << 2;

	switch( options->Parity ) {
	case FB_SERIAL_PARITY_NONE:
		break;
	case FB_SERIAL_PARITY_ODD:
		format |= 1 << 3;
		break;
	case FB_SERIAL_PARITY_EVEN:
		format |= (1 << 3) | (1 << 4);
		break;
	case FB_SERIAL_PARITY_MARK:
		format |= (1 << 3) | (2 << 4);
		break;
	case FB_SERIAL_PARITY_SPACE:
		format |= (1 << 3) | (3 << 4);
		break;
	default:
		return -1;
	}

	return format;
}

static int fb_hRiscosReadState( int *state )
{
	_kernel_swi_regs result;
	int error;

	if( state == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	error = fb_hRiscosSerialOp( 0, 0, -1, &result, NULL );
	if( error == FB_RTERROR_OK )
		*state = result.r[2];

	return error;
}

static int fb_hRiscosWriteState( int state )
{
	int old_state;
	int error;

	error = fb_hRiscosReadState( &old_state );
	if( error != FB_RTERROR_OK )
		return error;

	/* Bits 8 and above are read-only. Change only the documented low-byte
	   control bits and preserve any state owned by a newer serial module. */
	state = (old_state & ~0xff) | (state & 0xff);
	return fb_hRiscosSerialOp( 0, old_state ^ state, -1, NULL, NULL );
}

static int fb_hRiscosReadByte( unsigned char *byte, int *available )
{
	_kernel_swi_regs result;
	int carry;
	int error;

	if( (byte == NULL) || (available == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	error = fb_hRiscosSerialOp( 4, 0, 0, &result, &carry );
	if( error != FB_RTERROR_OK )
		return error;

	*available = !carry;
	if( !carry )
		*byte = (unsigned char)result.r[1];

	return FB_RTERROR_OK;
}

/* ------------------------------------------------------------------------- */
/* OPEN COM stream backend                                                   */
/* ------------------------------------------------------------------------- */

int fb_SerialOpen( FB_FILE *handle, int iPort, FB_SERIAL_OPTIONS *options,
	const char *pszDevice, void **ppvHandle )
{
	RISCOS_SERIAL_INFO *info;
	_kernel_swi_regs result;
	int baud_code;
	int format;
	int input_result;
	int state;
	int error;

	(void)handle;

	if( (options == NULL) || (ppvHandle == NULL) || (iPort < 0) ||
	    (iPort > 1) || (options->IRQNumber != 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	*ppvHandle = NULL;
	if( (iPort == 0) && (pszDevice != NULL) &&
	    (strcasecmp( pszDevice, "COM" ) != 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	baud_code = fb_hRiscosBaudCode( options->uiSpeed );
	format = fb_hRiscosFormat( options );
	if( (baud_code < 0) || (format < 0) || fb_riscos_serial_open )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	info = (RISCOS_SERIAL_INFO *)calloc( 1, sizeof( *info ) );
	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	error = fb_hRiscosReadState( &info->old_state );
	if( error == FB_RTERROR_OK )
		error = fb_hRiscosSerialOp( 1, -1, 0, &result, NULL );
	if( error == FB_RTERROR_OK ) {
		info->old_format = result.r[1];
		error = fb_hRiscosSerialOp( 5, -1, 0, &result, NULL );
	}
	if( error == FB_RTERROR_OK ) {
		info->old_rx_baud = result.r[1];
		error = fb_hRiscosSerialOp( 6, -1, 0, &result, NULL );
	}
	if( error == FB_RTERROR_OK )
		info->old_tx_baud = result.r[1];

	if( error != FB_RTERROR_OK ) {
		free( info );
		return fb_ErrorSetNum( error );
	}

	input_result = _kernel_osbyte( 2, 2, 0 );
	if( input_result < 0 ) {
		free( info );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	info->old_input_stream = input_result & 0xff;

	error = fb_hRiscosSerialOp( 1, format, 0, NULL, NULL );
	if( error == FB_RTERROR_OK )
		error = fb_hRiscosSerialOp( 5, baud_code, 0, NULL, NULL );
	if( error == FB_RTERROR_OK )
		error = fb_hRiscosSerialOp( 6, baud_code, 0, NULL, NULL );

	state = info->old_state;
	state &= ~(1 << 3); /* DTR on */
	if( options->DurationDSR == 0 )
		state |= 1 << 2;
	else
		state &= ~(1 << 2);
	if( options->DurationCD == 0 )
		state |= 1 << 1;
	else
		state &= ~(1 << 1);
	if( options->DurationCTS == 0 )
		state |= 1 << 4;
	else
		state &= ~(1 << 4);
	if( options->SuppressRTS ) {
		state |= (1 << 5) | (1 << 7);
	} else {
		state &= ~(1 << 5);
	}

	if( error == FB_RTERROR_OK )
		error = fb_hRiscosWriteState( state );

	if( error != FB_RTERROR_OK ) {
		(void)fb_hRiscosSerialOp( 6, info->old_tx_baud, 0, NULL, NULL );
		(void)fb_hRiscosSerialOp( 5, info->old_rx_baud, 0, NULL, NULL );
		(void)fb_hRiscosSerialOp( 1, info->old_format, 0, NULL, NULL );
		(void)fb_hRiscosWriteState( info->old_state );
		(void)_kernel_osbyte( 2, info->old_input_stream, 0 );
		free( info );
		return fb_ErrorSetNum( error );
	}

	info->pOptions = options;
	fb_riscos_serial_open = TRUE;
	*ppvHandle = info;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *pvHandle, fb_off_t *pLength )
{
	RISCOS_SERIAL_INFO *info = (RISCOS_SERIAL_INFO *)pvHandle;
	unsigned char byte;
	int available;
	int error;

	(void)handle;

	if( (info == NULL) || (pLength == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( !info->has_read_ahead ) {
		error = fb_hRiscosReadByte( &byte, &available );
		if( error != FB_RTERROR_OK ) {
			*pLength = 0;
			return fb_ErrorSetNum( error );
		}
		if( available ) {
			info->read_ahead = byte;
			info->has_read_ahead = TRUE;
		}
	}

	*pLength = info->has_read_ahead ? 1 : 0;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialWrite( FB_FILE *handle, void *pvHandle, const void *data,
	size_t length )
{
	RISCOS_SERIAL_INFO *info = (RISCOS_SERIAL_INFO *)pvHandle;
	const unsigned char *bytes = (const unsigned char *)data;
	size_t offset = 0;
	int start_time;
	int now;
	int carry;
	int error;
	int waiting = FALSE;

	(void)handle;

	if( (info == NULL) || ((data == NULL) && (length != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	while( offset < length ) {
		error = fb_hRiscosSerialOp( 3, bytes[offset], 0, NULL, &carry );
		if( error != FB_RTERROR_OK )
			return fb_ErrorSetNum( error );

		if( !carry ) {
			offset++;
			waiting = FALSE;
			continue;
		}

		if( !waiting ) {
			error = fb_hRiscosReadTime( &start_time );
			if( error != FB_RTERROR_OK )
				return fb_ErrorSetNum( error );
			waiting = TRUE;
			continue;
		}

		error = fb_hRiscosReadTime( &now );
		if( error != FB_RTERROR_OK )
			return fb_ErrorSetNum( error );
		if( (unsigned int)(now - start_time) >=
		    FB_RISCOS_SERIAL_TIMEOUT_CS )
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialRead( FB_FILE *handle, void *pvHandle, void *data,
	size_t *pLength )
{
	RISCOS_SERIAL_INFO *info = (RISCOS_SERIAL_INFO *)pvHandle;
	unsigned char *bytes = (unsigned char *)data;
	size_t count = 0;
	int start_time;
	int now;
	int available;
	int error;

	(void)handle;

	if( (info == NULL) || (pLength == NULL) ||
	    ((data == NULL) && (*pLength != 0)) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( *pLength == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	if( info->has_read_ahead ) {
		bytes[count++] = info->read_ahead;
		info->has_read_ahead = FALSE;
	}

	error = fb_hRiscosReadTime( &start_time );
	if( error != FB_RTERROR_OK )
		return fb_ErrorSetNum( error );

	while( count < *pLength ) {
		error = fb_hRiscosReadByte( &bytes[count], &available );
		if( error != FB_RTERROR_OK ) {
			*pLength = count;
			return fb_ErrorSetNum( error );
		}

		if( available ) {
			count++;
			continue;
		}

		error = fb_hRiscosReadTime( &now );
		if( error != FB_RTERROR_OK ) {
			*pLength = count;
			return fb_ErrorSetNum( error );
		}
		if( (unsigned int)(now - start_time) >=
		    FB_RISCOS_SERIAL_READ_TIMEOUT_CS )
			break;
	}

	*pLength = count;
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_SerialClose( FB_FILE *handle, void *pvHandle )
{
	RISCOS_SERIAL_INFO *info = (RISCOS_SERIAL_INFO *)pvHandle;
	int result = FB_RTERROR_OK;

	(void)handle;

	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( fb_hRiscosSerialOp( 6, info->old_tx_baud, 0, NULL, NULL ) !=
	    FB_RTERROR_OK )
		result = FB_RTERROR_FILEIO;
	if( fb_hRiscosSerialOp( 5, info->old_rx_baud, 0, NULL, NULL ) !=
	    FB_RTERROR_OK )
		result = FB_RTERROR_FILEIO;
	if( fb_hRiscosSerialOp( 1, info->old_format, 0, NULL, NULL ) !=
	    FB_RTERROR_OK )
		result = FB_RTERROR_FILEIO;
	if( fb_hRiscosWriteState( info->old_state ) != FB_RTERROR_OK )
		result = FB_RTERROR_FILEIO;
	if( _kernel_osbyte( 2, info->old_input_stream, 0 ) < 0 )
		result = FB_RTERROR_FILEIO;

	fb_riscos_serial_open = FALSE;
	free( info );

	return fb_ErrorSetNum( result );
}

/* end of io_serial.c */
