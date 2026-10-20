/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_write.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevScrnWrite( FB_FILE *handle, const void* value, size_t valuelen )
{
	fb_PrintBufferEx( value, valuelen, 0 );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

void fb_DevScrnInit_Write( void )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK( );
	if( FB_HANDLE_SCREEN->hooks->pfnWrite == NULL ) {
		FB_HANDLE_SCREEN->hooks->pfnWrite = fb_DevScrnWrite;
	}
	FB_UNLOCK( );
}

/* end of devices/dev_scrn_write.c */
