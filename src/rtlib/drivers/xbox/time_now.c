/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_now.c
 * Purpose: Return the Xbox local date and time as a BASIC serial value.
 * Responsibilities: Read the native calendar and use shared serial conversion.
 * This file intentionally does NOT set the clock or use libc localtime storage.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL double fb_Now( void )
{
	SYSTEMTIME local;

	/* nxdk provides this thread-safe kernel calendar conversion, whereas
	   its libc localtime() currently returns NULL. No shared tm needs locking. */
	GetLocalTime( &local );
	if( local.wYear < 1 || local.wYear > 9999 ||
	    local.wMonth < 1 || local.wMonth > 12 ||
	    local.wDay < 1 || local.wDay > 31 ||
	    local.wHour > 23 || local.wMinute > 59 || local.wSecond > 60 )
		return 0.0;
	return fb_DateSerial( local.wYear, local.wMonth, local.wDay ) +
	       fb_TimeSerial( local.wHour, local.wMinute, local.wSecond );
}

/* end of drivers/xbox/time_now.c */
