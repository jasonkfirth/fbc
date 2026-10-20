/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_sleepex.c
 * Purpose: sleep multiplexer function.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_SleepEx ( int msecs, int kind )
{
    switch( kind ) {
    case 0:
        fb_Sleep( msecs );
        break;
    case 1:
        fb_Delay( msecs );
        break;
    default:
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }
    return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of datetime/time_sleepex.c */
