/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: unix/io_serial_control.c

    Purpose:

        Implement portable raw serial controls for file-descriptor based
        termios devices.

    Responsibilities:

        - translate TTY modem bits into fbcom.bi line flags
        - query receive and transmit queue lengths where the driver permits
        - set RTS and DTR, assert break, and purge terminal queues
        - report runtime capabilities without inventing unsupported state

    This file intentionally does NOT contain:

        - serial-device opening or termios framing configuration
        - AROS serial.device or RISC OS SWI handling
        - Windows or DOS serial APIs

    External API behavior:

        TIOCMGET and TIOCMSET operate on the driver modem-bit word. Some USB
        serial drivers reject one or both requests; ComGetStatus leaves those
        fields unsupported instead of treating that device limitation as a
        failure of the complete status query. tcflush follows the POSIX queue
        selectors and discards only the queues selected by the caller.
*/

#include "../fb.h"
#include "../io_serial_private.h"

#include <limits.h>
#include <sys/ioctl.h>
#include <termios.h>

/* ------------------------------------------------------------------------- */
/* Native modem-bit translation                                              */
/* ------------------------------------------------------------------------- */

static unsigned int fb_hSerialLinesFromNative( int native_lines )
{
	unsigned int lines = 0;

#ifdef TIOCM_CTS
	if( (native_lines & TIOCM_CTS) != 0 )
		lines |= FB_COM_LINE_CTS;
#endif
#ifdef TIOCM_DSR
	if( (native_lines & TIOCM_DSR) != 0 )
		lines |= FB_COM_LINE_DSR;
#endif
#ifdef TIOCM_CAR
	if( (native_lines & TIOCM_CAR) != 0 )
		lines |= FB_COM_LINE_DCD;
#elif defined TIOCM_CD
	if( (native_lines & TIOCM_CD) != 0 )
		lines |= FB_COM_LINE_DCD;
#elif defined TIOCM_DCD
	if( (native_lines & TIOCM_DCD) != 0 )
		lines |= FB_COM_LINE_DCD;
#endif
#ifdef TIOCM_RI
	if( (native_lines & TIOCM_RI) != 0 )
		lines |= FB_COM_LINE_RI;
#elif defined TIOCM_RNG
	if( (native_lines & TIOCM_RNG) != 0 )
		lines |= FB_COM_LINE_RI;
#endif
#ifdef TIOCM_RTS
	if( (native_lines & TIOCM_RTS) != 0 )
		lines |= FB_COM_LINE_RTS;
#endif
#ifdef TIOCM_DTR
	if( (native_lines & TIOCM_DTR) != 0 )
		lines |= FB_COM_LINE_DTR;
#endif

	return lines;
}

static int fb_hSerialLinesToNative( unsigned int lines )
{
	int native_lines = 0;

#ifdef TIOCM_RTS
	if( (lines & FB_COM_LINE_RTS) != 0 )
		native_lines |= TIOCM_RTS;
#endif
#ifdef TIOCM_DTR
	if( (lines & FB_COM_LINE_DTR) != 0 )
		native_lines |= TIOCM_DTR;
#endif

	return native_lines;
}

static unsigned int fb_hSerialQueueLength( int queued )
{
	if( queued <= 0 )
		return 0;

	return (unsigned int)queued;
}

/* ------------------------------------------------------------------------- */
/* POSIX serial control backend                                              */
/* ------------------------------------------------------------------------- */

int fb_SerialGetStatus( FB_FILE *handle, void *serial_handle,
	FB_COM_STATUS *status )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	int queued;
	int native_lines;

	(void)handle;

	if( (info == NULL) || (status == NULL) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	memset( status, 0, sizeof( *status ) );
	status->capabilities |= FB_COM_CAP_PURGE_RX | FB_COM_CAP_PURGE_TX;

#if defined TIOCSBRK && defined TIOCCBRK
	status->capabilities |= FB_COM_CAP_BREAK;
#endif

#ifdef TIOCMGET
	native_lines = 0;
	if( ioctl( info->sfd, TIOCMGET, &native_lines ) == 0 ) {
		status->lines = fb_hSerialLinesFromNative( native_lines );
		status->capabilities |= FB_COM_CAP_INPUT_LINES;
#if defined TIOCMSET && defined TIOCM_RTS && defined TIOCM_DTR
		status->capabilities |= FB_COM_CAP_OUTPUT_LINES;
#endif
	}
#else
	(void)native_lines;
#endif

#ifdef FIONREAD
	queued = 0;
	if( ioctl( info->sfd, FIONREAD, &queued ) == 0 ) {
		status->rx_queued = fb_hSerialQueueLength( queued );
		status->capabilities |= FB_COM_CAP_RX_QUEUE;
	}
#else
	(void)queued;
#endif

#ifdef TIOCOUTQ
	queued = 0;
	if( ioctl( info->sfd, TIOCOUTQ, &queued ) == 0 ) {
		status->tx_queued = fb_hSerialQueueLength( queued );
		status->capabilities |= FB_COM_CAP_TX_QUEUE;
	}
#endif

	return FB_RTERROR_OK;
}

int fb_SerialSetLines( FB_FILE *handle, void *serial_handle,
	unsigned int mask, unsigned int values )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	int native_lines;
	int native_mask;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

#if defined TIOCMGET && defined TIOCMSET && defined TIOCM_RTS && \
    defined TIOCM_DTR
	native_lines = 0;
	native_mask = fb_hSerialLinesToNative( mask );

	if( ioctl( info->sfd, TIOCMGET, &native_lines ) != 0 )
		return FB_RTERROR_FILEIO;

	native_lines &= ~native_mask;
	native_lines |= fb_hSerialLinesToNative( values & mask );

	if( ioctl( info->sfd, TIOCMSET, &native_lines ) != 0 )
		return FB_RTERROR_FILEIO;

	return FB_RTERROR_OK;
#else
	(void)mask;
	(void)values;
	(void)native_lines;
	(void)native_mask;
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
#endif
}

int fb_SerialSetBreak( FB_FILE *handle, void *serial_handle, int enabled )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

#if defined TIOCSBRK && defined TIOCCBRK
	if( enabled ) {
		if( ioctl( info->sfd, TIOCSBRK ) != 0 )
			return FB_RTERROR_FILEIO;
	} else {
		if( ioctl( info->sfd, TIOCCBRK ) != 0 )
			return FB_RTERROR_FILEIO;
	}

	return FB_RTERROR_OK;
#else
	(void)enabled;
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
#endif
}

int fb_SerialPurge( FB_FILE *handle, void *serial_handle,
	unsigned int queues )
{
	LINUX_SERIAL_INFO *info = (LINUX_SERIAL_INFO *)serial_handle;
	int selector;

	(void)handle;

	if( info == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( queues == (FB_COM_PURGE_RX | FB_COM_PURGE_TX) )
		selector = TCIOFLUSH;
	else if( queues == FB_COM_PURGE_RX )
		selector = TCIFLUSH;
	else if( queues == FB_COM_PURGE_TX )
		selector = TCOFLUSH;
	else
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	if( tcflush( info->sfd, selector ) != 0 )
		return FB_RTERROR_FILEIO;

	return FB_RTERROR_OK;
}

/* end of unix/io_serial_control.c */
