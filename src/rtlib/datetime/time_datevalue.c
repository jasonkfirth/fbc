/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_datevalue.c
 * Purpose: datevalue function.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_DateValue ( FBSTRING *s )
{
    int year;
    int month;
    int day;
    int succeeded = fb_DateParse( s, &day, &month, &year );

    fb_hStrDelTemp( s );

    if( !succeeded ) {
        fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        return 0;
    }

    fb_ErrorSetNum( FB_RTERROR_OK );

	return fb_DateSerial( year, month, day );
}

/* end of datetime/time_datevalue.c */
