/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_readline.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevScrnReadLine( FB_FILE *handle, FBSTRING *dst )
{
	return fb_LineInput( NULL, dst, -1, FALSE, FALSE, TRUE );
}

void fb_DevScrnInit_ReadLine( void )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK( );
	if( FB_HANDLE_SCREEN->hooks->pfnReadLine == NULL ) {
		FB_HANDLE_SCREEN->hooks->pfnReadLine = fb_DevScrnReadLine;
	}
	FB_UNLOCK( );
}

/* end of devices/dev_scrn_readline.c */
