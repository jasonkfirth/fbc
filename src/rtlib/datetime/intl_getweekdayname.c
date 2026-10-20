/*
 * Project: FreeBASIC runtime library
 * File: datetime/intl_getweekdayname.c
 * Purpose: get weekday name.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static const char *pszWeekdayNamesLong[7] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

static const char *pszWeekdayNamesShort[7] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

/*:::::*/
FBSTRING *fb_IntlGetWeekdayName( int weekday, int short_names, int disallow_localized )
{
    FBSTRING *res;

    if( weekday < 1 || weekday > 7 )
        return NULL;

    if( fb_I18nGet() && !disallow_localized ) {
        res = fb_DrvIntlGetWeekdayName( weekday, short_names );
        if( res!=NULL )
            return res;
    }
    if( short_names ) {
        res = fb_StrAllocTempDescZ( pszWeekdayNamesShort[weekday-1] );
    } else {
        res = fb_StrAllocTempDescZ( pszWeekdayNamesLong[weekday-1] );
    }
    if( res==&__fb_ctx.null_desc )
        return NULL;
    return res;
}

/* end of datetime/intl_getweekdayname.c */
