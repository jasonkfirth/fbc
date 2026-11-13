/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_parsetime.c
 * Purpose: parse a string time.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <ctype.h>

/* strtol has no input-span limit. Parse the small decimal time fields here
   so neither a missing terminator nor an oversized value can escape the
   caller's byte span or overflow before its range is checked. */
static int fb_hTimeNumber( const char **text, const char *text_end,
                          unsigned int maximum, int *number )
{
    const char *p = *text, *digits;
    unsigned int value = 0;
    int negative = FALSE;

    while( p < text_end && isspace( FB_CHAR_TO_INT( *p ) ) )
        ++p;
    if( p < text_end && (*p == '-' || *p == '+') ) {
        negative = (*p == '-');
        ++p;
    }
    digits = p;
    while( p < text_end && *p >= '0' && *p <= '9' ) {
        value = value * 10 + (unsigned int)(*p - '0');
        if( value > maximum )
            return FALSE;
        ++p;
    }
    if( p == digits || (negative && value != 0) )
        return FALSE;
    *number = (int)value;
    *text = p;
    return TRUE;
}

/*:::::*/
static int fb_hCheckAMPM( const char *text, const char *text_end, size_t *pLength )
{
    const char *text_start = text;
    int result = 0;

    /* skip WS */
    while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
        ++text;

    if( text == text_end )
        return 0;
    switch( *text ) {
    case 'a':
    case 'A':
        result = 1;
        ++text;
        break;
    case 'p':
    case 'P':
        result = 2;
        ++text;
        break;
    }
    if( result!=0 ) {
        char ch = (text < text_end) ? *text : 0;
        if( ch==0 ) {
            // ignore
        } else if( ch=='m' || ch=='M' ) {
            // everything's fine
            ++text;
        } else {
            result = 0;
        }
    }
    if( result!=0 ) {
        if( text < text_end && isalpha( FB_CHAR_TO_INT( *text ) ) )
            result = 0;
    }
    if( result!=0 ) {
        /* skip WS */
        while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
            ++text;
        if( pLength )
            *pLength = text - text_start;
    }
    return result;
}

/*:::::*/
int fb_hTimeParse( const char *text, size_t text_len, int *pHour, int *pMinute, int *pSecond, size_t *pLength )
{
    size_t length = 0;
    const char *text_start = text;
    int am_pm = 0;
    int result = FALSE;
    int hour = 0, minute = 0, second = 0;
    const char *text_end;

    if( text == NULL || text_len == 0 || text_len > (size_t)PTRDIFF_MAX )
        return FALSE;
    text_end = text + text_len;

    if( fb_hTimeNumber( &text, text_end, 23, &hour ) ) {
        int is_ampm_hour = ( hour >= 1 && hour <= 12 );
        /* skip white spaces */
        while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
            ++text;
        if( text < text_end && *text==':' ) {
            ++text;
            if( fb_hTimeNumber( &text, text_end, 59, &minute ) ) {
                while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                    ++text;

                result = TRUE;
                if( text < text_end && *text==':' ) {
                    ++text;
                    if( !fb_hTimeNumber( &text, text_end, 59, &second ) ) {
                        result = FALSE;
                    }
                }
                if( result && is_ampm_hour ) {
                    am_pm = fb_hCheckAMPM( text, text_end, &length );
                    if( am_pm ) {
                        text += length;
                    }
                }
            }
        } else if( is_ampm_hour ) {
            am_pm = fb_hCheckAMPM( text, text_end, &length );
            if( am_pm ) {
                text += length;
                result = TRUE;
            }
        }
    }
    if( result ) {
        if( am_pm ) {
            /* test for AM/PM */
            if( hour==12 ) {
                if( am_pm==1 )
                    hour -= 12;
            } else {
                if( am_pm==2 )
                    hour += 12;
            }
        }
        /* Update used length */
        length = text - text_start;
    }

    if( result ) {
        if( pHour )
            *pHour = hour;
        if( pMinute )
            *pMinute = minute;
        if( pSecond )
            *pSecond = second;
        if( pLength )
            *pLength = length;
    }

    return result;
}

/* end of datetime/time_parsetime.c */
