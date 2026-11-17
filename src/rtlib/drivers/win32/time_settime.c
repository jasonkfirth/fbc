/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/time_settime.c
 * Purpose: Provide time settime runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <limits.h>
#include <windows.h>

int fb_hSetTime( int h, int m, int s )
{
	SYSTEMTIME st;

	/* SYSTEMTIME stores these fields in WORDs. Reject values that would
	 * otherwise wrap when assigned to the Windows structure. */
	if( (h < 0) || (h > USHRT_MAX) ||
		(m < 0) || (m > USHRT_MAX) ||
		(s < 0) || (s > USHRT_MAX) )
	{
		return -1;
	}

	/* get current local time and date */
	GetLocalTime( &st );

	/* set time fields */
	st.wHour = (WORD)h;
	st.wMinute = (WORD)m;
	st.wSecond = (WORD)s;

	/* set system time relative to local time zone */
	if( SetLocalTime( &st ) == 0 )
	{
		return -1;
	}

	/* send WM_TIMECHANGE to all top-level windows on NT and 95/98/Me
	 * (_not_ on 2K/XP etc.) */
	/*if ((GetVersion() & 0xFF) == 4)
		SendMessage(HWND_BROADCAST, WM_TIMECHANGE, 0, 0);*/

	return 0;
}

/* end of drivers/win32/time_settime.c */
