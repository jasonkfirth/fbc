/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_timeserial.c
 * Purpose: timeserial function.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL double fb_TimeSerial ( int hour, int minute, int second )
{
    double dblHour = 1.0 * (double) hour / 24.0;
    double dblMinute = 1.0 * minute / (24.0 * 60.0);
    double dblSecond = 1.0 * second / (24.0 * 60.0 * 60.0);
    return dblHour + dblMinute + dblSecond;
}

/* end of datetime/time_timeserial.c */
