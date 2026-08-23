/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: dev_com_control.c

    Purpose:

        Connect the public fbcom.bi functions to the platform backend that
        owns an already-open OPEN COM device.

    Responsibilities:

        - validate and dereference BASIC file numbers
        - reject non-serial handles and invalid control masks
        - serialize raw control operations against normal COM I/O and close
        - keep caller-owned status deterministic on every failure path

    This file intentionally does NOT contain:

        - operating-system serial calls
        - OPEN COM option parsing or device creation
        - platform-native handle assumptions
*/

#include "fb.h"
#include "dev_com_private.h"

/* ------------------------------------------------------------------------- */
/* Serial handle validation                                                  */
/* ------------------------------------------------------------------------- */

static DEV_COM_INFO *fb_hComControlGetInfo( FB_FILE *handle )
{
	DEV_COM_INFO *info;

	if( (handle == NULL) || (handle->type != FB_FILE_TYPE_SERIAL) ||
	    (handle->opaque == NULL) )
		return NULL;

	info = (DEV_COM_INFO *)handle->opaque;
	if( info->hSerial == NULL )
		return NULL;

	return info;
}

/* ------------------------------------------------------------------------- */
/* Public portable serial control API                                        */
/* ------------------------------------------------------------------------- */

FBCALL int fb_ComGetStatus( int file_number, FB_COM_STATUS *status )
{
	FB_FILE *handle;
	DEV_COM_INFO *info;
	int result;

	if( status == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	memset( status, 0, sizeof( *status ) );

	FB_LOCK();
	handle = FB_HANDLE_DEREF( FB_FILE_TO_HANDLE( file_number ) );

	info = fb_hComControlGetInfo( handle );
	if( info == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_SerialGetStatus( handle, info->hSerial, status );

	FB_UNLOCK();

	return fb_ErrorSetNum( result );
}

FBCALL int fb_ComSetLines( int file_number, unsigned int mask,
	unsigned int values )
{
	const unsigned int valid_lines = FB_COM_LINE_RTS | FB_COM_LINE_DTR;
	FB_FILE *handle;
	DEV_COM_INFO *info;
	int result;

	if( (mask == 0) || ((mask & ~valid_lines) != 0) ||
	    ((values & ~valid_lines) != 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	FB_LOCK();
	handle = FB_HANDLE_DEREF( FB_FILE_TO_HANDLE( file_number ) );

	info = fb_hComControlGetInfo( handle );
	if( info == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_SerialSetLines( handle, info->hSerial, mask, values );

	FB_UNLOCK();

	return fb_ErrorSetNum( result );
}

FBCALL int fb_ComSetBreak( int file_number, int enabled )
{
	FB_FILE *handle;
	DEV_COM_INFO *info;
	int result;

	FB_LOCK();
	handle = FB_HANDLE_DEREF( FB_FILE_TO_HANDLE( file_number ) );

	info = fb_hComControlGetInfo( handle );
	if( info == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_SerialSetBreak( handle, info->hSerial, enabled != 0 );

	FB_UNLOCK();

	return fb_ErrorSetNum( result );
}

FBCALL int fb_ComPurge( int file_number, unsigned int queues )
{
	const unsigned int valid_queues = FB_COM_PURGE_RX | FB_COM_PURGE_TX;
	FB_FILE *handle;
	DEV_COM_INFO *info;
	int result;

	if( (queues == 0) || ((queues & ~valid_queues) != 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	FB_LOCK();
	handle = FB_HANDLE_DEREF( FB_FILE_TO_HANDLE( file_number ) );

	info = fb_hComControlGetInfo( handle );
	if( info == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_SerialPurge( handle, info->hSerial, queues );

	FB_UNLOCK();

	return fb_ErrorSetNum( result );
}

/* end of dev_com_control.c */
