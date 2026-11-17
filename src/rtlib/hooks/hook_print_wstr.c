/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_print_wstr.c
 * Purpose: print wstring entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintBufferWstrEx( const FB_WCHAR *buffer, size_t len, int mask )
{
	FB_LOCK();

	if( fb_ctx.hooks.printbuffwproc ) {
		fb_ctx.hooks.printbuffwproc( buffer, len, mask );
	} else {
		fb_ConsolePrintBufferWstrEx( buffer, len, mask );
	}

	FB_UNLOCK();

}

/*:::::*/
FBCALL void fb_PrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	ssize_t length = fb_wstr_Len( buffer );
	if( length < 0 ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return;
	}
	fb_PrintBufferWstrEx( buffer, (size_t)length, mask );
}

/* end of hooks/hook_print_wstr.c */
