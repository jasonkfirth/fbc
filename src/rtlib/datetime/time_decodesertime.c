/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_decodesertime.c
 * Purpose: functions to decode a serial time number.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <math.h>

/*:::::*/
FBCALL void fb_hTimeDecodeSerial ( double serial,
                                   int *pHour, int *pMinute, int *pSecond,
                                   int use_qb_hack )
{
    int hour, minute, second;
    double dblFixValue = fb_FIXDouble( serial );

    serial -= dblFixValue;
    if( fb_hSign( serial ) == -1 ) {
        if( use_qb_hack ) {
            /* Test for both 0.0 and -0.0 because FPUs may handle this as
             * different values ... */
	if( fpclassify(dblFixValue) == FP_ZERO ) {
                /* QB quirk ! */
                serial = -serial;
            } else {
                serial = (double)((long double)serial + 1.0l);
            }
        } else {
            serial = (double)((long double)serial + 1.0l);
        }
    }

    /* The inaccuracies of the IEEE floating point data types ... */
    serial = (double)((long double)serial + 0.000000001l);

    serial = (double)((long double)serial * 24.0l);
    hour = (int) serial;
    serial -= hour;
    serial = (double)((long double)serial * 60.0l);
    minute = (int) serial;
    serial -= minute;
    serial = (double)((long double)serial * 60.0l);
    second = (int) serial;

    if( pHour )
        *pHour = hour;
    if( pMinute )
        *pMinute = minute;
    if( pSecond )
        *pSecond = second;
}

FBCALL int fb_Hour( double serial )
{
    int hour;
    fb_hTimeDecodeSerial( serial, &hour, NULL, NULL, TRUE );
    return hour;
}

FBCALL int fb_Minute( double serial )
{
    int minute;
    fb_hTimeDecodeSerial( serial, NULL, &minute, NULL, TRUE );
    return minute;
}

FBCALL int fb_Second( double serial )
{
    int second;
    fb_hTimeDecodeSerial( serial, NULL, NULL, &second, TRUE );
    return second;
}

/* end of datetime/time_decodesertime.c */
