/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_time.c
 * Purpose: Return the Xbox local clock time for TIME$.
 * Responsibilities: Read the native clock and format hours, minutes, seconds.
 * This file intentionally does NOT set the clock or implement libc localtime.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL FBSTRING_ACCESS *fb_Time( void )
{
	SYSTEMTIME local;
	char text[9];
	FBSTRING_ACCESS *dst = &fb_ctx.null_desc;

	/* Use nxdk's implemented native calendar API; its libc localtime()
	   currently returns NULL even for a valid time_t. */
	GetLocalTime( &local );
	if( local.wHour > 23 || local.wMinute > 59 || local.wSecond > 60 )
		return dst;
	if( snprintf( text, sizeof(text), "%02d:%02d:%02d",
	              (int)local.wHour, (int)local.wMinute, (int)local.wSecond ) != 8 )
		return dst;

	FB_LOCK();
	dst = fb_hStrAllocTemp( NULL, 8 );
	if( dst != NULL )
		memcpy( dst->data, text, sizeof(text) );
	else
		dst = &fb_ctx.null_desc;
	FB_UNLOCK();
	return dst;
}

/* end of drivers/xbox/time_time.c */
