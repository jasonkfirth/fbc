/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_monthname.c
 * Purpose: returns the month name.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL FBSTRING_ACCESS *fb_MonthName( int month, int abbreviation )
{
    FBSTRING_ACCESS *res;

    if( month < 1 || month > 12 ) {
        fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
        return &fb_ctx.null_desc;
    }

    fb_ErrorSetNum( FB_RTERROR_OK );

    res = fb_IntlGetMonthName( month, abbreviation, FALSE );
    if( res==NULL ) {
        fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
        res = &fb_ctx.null_desc;
    }

    return res;
}

/* end of datetime/time_monthname.c */
