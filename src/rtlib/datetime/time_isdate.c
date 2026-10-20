/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_isdate.c
 * Purpose: isdate function.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_IsDate ( FBSTRING *s )
{
    int year;
    int month;
    int day;
    int succeeded = fb_DateParse( s, &day, &month, &year );

    fb_hStrDelTemp( s );

    if( !succeeded )
			return 0;

		return -1;
}

/* end of datetime/time_isdate.c */
