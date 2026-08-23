/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: riscos/io_serial_control.c

    Purpose:

        Implement fbcom.bi modem status and raw RTS/DTR control through
        OS_SerialOp on RISC OS.

    Responsibilities:

        - translate OS_SerialOp state bits into portable line flags
        - place RTS in manual mode before changing its electrical state
        - discard buffered receive data without changing stream routing

    This file intentionally does NOT contain:

        - serial stream reads, writes, or framing configuration
        - timed break pulses represented as persistent break state
        - add-on multi-port serial driver SWIs
*/

#include "../fb.h"
#include "../io_serial_private.h"

#include <kernel.h>
#include <swis.h>

/* ------------------------------------------------------------------------- */
/* Native state access                                                       */
/* ------------------------------------------------------------------------- */

static int fb_hRiscosControlSerialOp( int reason, int value, int mask,
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

static int fb_hRiscosControlReadState( int *state )
{
	_kernel_swi_regs result;
	int error;

	if( state == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	error = fb_hRiscosControlSerialOp( 0, 0, -1, &result, NULL );
	if( error == FB_RTERROR_OK )
		*state = result.r[2];

	return error;
}

static int fb_hRiscosControlWriteState( int old_state, int new_state )
{
	new_state = (old_state & ~0xff) | (new_state & 0xff);
	return fb_hRiscosControlSerialOp( 0, old_state ^ new_state, -1,
		NULL, NULL );
}

/* ------------------------------------------------------------------------- */
/* Portable serial control backend                                           */
/* ------------------------------------------------------------------------- */

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	int state;
	int error;

	(void)handle;

	if( (serial_handle == NULL) || (status == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	memset( status, 0, sizeof( *status ) );
	error = fb_hRiscosControlReadState( &state );
	if( error != FB_RTERROR_OK )
		return error;

	status->capabilities = FB_COM_CAP_INPUT_LINES |
		FB_COM_CAP_OUTPUT_LINES | FB_COM_CAP_PURGE_RX;

	/* The RISC OS state word uses zero for each active-low modem signal. */
	if( (state & (1 << 21)) == 0 )
		status->lines |= FB_COM_LINE_CTS;
	if( (state & (1 << 19)) == 0 )
		status->lines |= FB_COM_LINE_DSR;
	if( (state & (1 << 18)) == 0 )
		status->lines |= FB_COM_LINE_DCD;
	if( (state & (1 << 20)) == 0 )
		status->lines |= FB_COM_LINE_RI;
	if( (state & (1 << 7)) == 0 )
		status->lines |= FB_COM_LINE_RTS;
	if( (state & (1 << 3)) == 0 )
		status->lines |= FB_COM_LINE_DTR;

	return FB_RTERROR_OK;
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	int old_state;
	int new_state;
	int error;

	(void)handle;

	if( serial_handle == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	error = fb_hRiscosControlReadState( &old_state );
	if( error != FB_RTERROR_OK )
		return error;

	new_state = old_state;
	if( (mask & FB_COM_LINE_DTR) != 0 ) {
		if( (values & FB_COM_LINE_DTR) != 0 )
			new_state &= ~(1 << 3);
		else
			new_state |= 1 << 3;
	}

	if( (mask & FB_COM_LINE_RTS) != 0 ) {
		/* Bit 5 disables automatic RTS handshaking. Bit 7 then drives the
		   active-low RTS line directly. */
		new_state |= 1 << 5;
		if( (values & FB_COM_LINE_RTS) != 0 )
			new_state &= ~(1 << 7);
		else
			new_state |= 1 << 7;
	}

	return fb_hRiscosControlWriteState( old_state, new_state );
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	(void)handle;
	(void)serial_handle;
	(void)enabled;

	/* OS_SerialOp 2 sends one timed pulse and cannot assert or clear break. */
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	_kernel_swi_regs result;
	int carry;

	(void)handle;

	if( (serial_handle == NULL) || (queues != FB_COM_PURGE_RX) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	do {
		if( fb_hRiscosControlSerialOp( 4, 0, 0, &result, &carry ) !=
		    FB_RTERROR_OK )
			return FB_RTERROR_FILEIO;
	} while( !carry );

	((RISCOS_SERIAL_INFO *)serial_handle)->has_read_ahead = FALSE;
	return FB_RTERROR_OK;
}

/* end of riscos/io_serial_control.c */
