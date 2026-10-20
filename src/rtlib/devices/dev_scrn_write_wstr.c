/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_write_wstr.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevScrnWriteWstr( FB_FILE *handle, const FB_WCHAR* value, size_t valuelen )
{
	fb_PrintBufferWstrEx( value, valuelen, 0 );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

void fb_DevScrnInit_WriteWstr( void )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK( );
	if( FB_HANDLE_SCREEN->hooks->pfnWriteWstr == NULL ) {
		FB_HANDLE_SCREEN->hooks->pfnWriteWstr = fb_DevScrnWriteWstr;
	}
	FB_UNLOCK( );
}

/* end of devices/dev_scrn_write_wstr.c */
