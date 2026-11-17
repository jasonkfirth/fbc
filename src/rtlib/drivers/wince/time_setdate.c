/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/time_setdate.c
 * Purpose: Provide time setdate runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

int fb_hSetDate( int y, int m, int d )
{
	if( (y < 1601) || (y > 9999) ||
	    (m < 1) || (m > 12) || (d < 1) || (d > 31) )
		return -1;

	/* get current local time and date */
	SYSTEMTIME st;
	GetLocalTime( &st );

	/* set time fields */
	st.wYear = (WORD)y;
	st.wMonth = (WORD)m;
	st.wDay = (WORD)d;

	/* set system time relative to local time zone */
	if( SetLocalTime( &st ) == 0) {
		return -1;
	}

	/* send WM_TIMECHANGE to all top-level windows on NT and 95/98/Me
	 * (_not_ on 2K/XP etc.) */
	/* if ((GetVersion() & 0xFF) == 4)
		SendMessage(HWND_BROADCAST, WM_TIMECHANGE, 0, 0); */

	return 0;
}

/* end of drivers/wince/time_setdate.c */
