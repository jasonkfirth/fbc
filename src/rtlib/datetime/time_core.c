/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_core.c
 * Purpose: core time/date functions.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

#include <limits.h>

/*:::::*/
int fb_hTimeLeap( int year )
{
    if( ( year % 400 ) == 0 )
        return 1;
    if( ( year % 100 ) == 0 )
        return 0;
    return ( ( ( year & 3 ) == 0 ) ? 1 : 0 );
}

/*:::::*/
int fb_hTimeDaysInMonth( int month, int year )
{
    static const int days[] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    DBG_ASSERT(month >= 1 && month <= 12 );
    if( month==2 )
        return days[month-1] + fb_hTimeLeap( year );
    return days[month-1];
}

/*:::::*/
void fb_hNormalizeDate( int *pDay, int *pMonth, int *pYear )
{
	int64_t year;
	int64_t month_index;
	int64_t day;
	int64_t cycles;
	int month;
	int month_remainder;
	int days_in_month;

	if( (pDay == NULL) || (pMonth == NULL) || (pYear == NULL) )
		return;

	day = *pDay;
	month_index = ((int64_t)*pYear * 12) + ((int64_t)*pMonth - 1);
	year = month_index / 12;
	month_remainder = (int)(month_index % 12);
	if( month_remainder < 0 ) {
		month_remainder += 12;
		--year;
	}
	month = month_remainder + 1;

	/* Gregorian month and weekday patterns repeat every 400 years. */
	if( day > 1 ) {
		cycles = (day - 1) / 146097;
		day -= cycles * 146097;
		year += cycles * 400;
	} else if( day < 1 ) {
		cycles = (1 - day) / 146097;
		day += cycles * 146097;
		year -= cycles * 400;
	}

	if( (year < INT_MIN) || (year > INT_MAX) )
		return;

	while( day < 1 ) {
		--month;
		if( month == 0 ) {
			month = 12;
			--year;
			if( (year < INT_MIN) || (year > INT_MAX) )
				return;
		}
		days_in_month = fb_hTimeDaysInMonth( month, (int)year );
		day += days_in_month;
	}

	for( ;; ) {
		days_in_month = fb_hTimeDaysInMonth( month, (int)year );
		if( day <= days_in_month )
			break;
		day -= days_in_month;
		++month;
		if( month == 13 ) {
			month = 1;
			++year;
			if( (year < INT_MIN) || (year > INT_MAX) )
				return;
		}
	}

	*pDay = (int)day;
	*pMonth = month;
	*pYear = (int)year;
}

/*:::::*/
int fb_hTimeGetIntervalType( FBSTRING_ACCESS *interval )
{
    int result = FB_TIME_INTERVAL_INVALID;

    if( interval!=NULL && interval->data!=NULL ) {
        if( strcmp( interval->data, "yyyy" )==0 ) {
            result = FB_TIME_INTERVAL_YEAR;
        } else if( strcmp( interval->data, "q" )==0 ) {
            result = FB_TIME_INTERVAL_QUARTER;
        } else if( strcmp( interval->data, "m" )==0 ) {
            result = FB_TIME_INTERVAL_MONTH;
        } else if( strcmp( interval->data, "y" )==0 ) {
            result = FB_TIME_INTERVAL_DAY_OF_YEAR;
        } else if( strcmp( interval->data, "d" )==0 ) {
            result = FB_TIME_INTERVAL_DAY;
        } else if( strcmp( interval->data, "w" )==0 ) {
            result = FB_TIME_INTERVAL_WEEKDAY;
        } else if( strcmp( interval->data, "ww" )==0 ) {
            result = FB_TIME_INTERVAL_WEEK_OF_YEAR;
        } else if( strcmp( interval->data, "h" )==0 ) {
            result = FB_TIME_INTERVAL_HOUR;
        } else if( strcmp( interval->data, "n" )==0 ) {
            result = FB_TIME_INTERVAL_MINUTE;
        } else if( strcmp( interval->data, "s" )==0 ) {
            result = FB_TIME_INTERVAL_SECOND;
        }
    }

    return result;
}

/* end of datetime/time_core.c */
