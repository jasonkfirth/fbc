/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_readline_wstr.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevScrnReadLineWstr( FB_FILE *handle, FB_WCHAR *dst, ssize_t dst_chars )
{
	(void)handle;
	return fb_LineInputWstr( NULL, dst, dst_chars, FALSE, TRUE );
}

void fb_DevScrnInit_ReadLineWstr( void )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK( );
	if( FB_HANDLE_SCREEN->hooks->pfnReadLineWstr == NULL ) {
		FB_HANDLE_SCREEN->hooks->pfnReadLineWstr = fb_DevScrnReadLineWstr;
	}
	FB_UNLOCK( );
}

/* end of devices/dev_scrn_readline_wstr.c */
