/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_date.c
 * Purpose: Return the Xbox local calendar date for DATE$.
 * Responsibilities: Read the native clock and format month-day-year.
 * This file intentionally does NOT set the clock or implement libc localtime.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL FBSTRING_ACCESS *fb_Date( void )
{
	SYSTEMTIME local;
	char text[11];
	FBSTRING_ACCESS *dst = &fb_ctx.null_desc;

	/* nxdk's libc localtime() is a stub. GetLocalTime() uses the Xbox
	   kernel clock and timezone conversion, without shared tm storage. */
	GetLocalTime( &local );
	if( local.wYear < 1 || local.wYear > 9999 ||
	    local.wMonth < 1 || local.wMonth > 12 ||
	    local.wDay < 1 || local.wDay > 31 )
		return dst;
	if( snprintf( text, sizeof(text), "%02d-%02d-%04d",
	              (int)local.wMonth, (int)local.wDay, (int)local.wYear ) != 10 )
		return dst;

	FB_LOCK();
	dst = fb_hStrAllocTemp( NULL, 10 );
	if( dst != NULL )
		memcpy( dst->data, text, sizeof(text) );
	else
		dst = &fb_ctx.null_desc;
	FB_UNLOCK();
	return dst;
}

/* end of drivers/xbox/time_date.c */
