/*
 * Project: FreeBASIC runtime library
 * File: datetime/time_parsedate.c
 * Purpose: parse a string date.
 * Responsibilities: Date arithmetic, time conversion and locale queries.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <ctype.h>
#include <limits.h>

/* Date fields have a C-int result and must stay inside the caller's span.
   Accumulate only after checking the next decimal digit against that range. */
static int fb_hDateNumber( const char *text, const char *text_end,
                          const char **end_text, int *number )
{
    const char *p = text, *digits;
    unsigned int value = 0, maximum, digit;
    int negative = FALSE;

    while( p < text_end && isspace( FB_CHAR_TO_INT( *p ) ) )
        ++p;
    if( p < text_end && (*p == '-' || *p == '+') ) {
        negative = (*p == '-');
        ++p;
    }
    maximum = (unsigned int)INT_MAX + (unsigned int)negative;
    digits = p;
    while( p < text_end && *p >= '0' && *p <= '9' ) {
        digit = (unsigned int)(*p - '0');
        if( value > (maximum - digit) / 10 )
            return FALSE;
        value = value * 10 + digit;
        ++p;
    }
    if( p == digits )
        return FALSE;
    if( negative ) {
        if( value > (unsigned int)INT_MAX )
            *number = INT_MIN;
        else
            *number = -(int)value;
    } else {
        *number = (int)value;
    }
    *end_text = p;
    return TRUE;
}

/*:::::*/
static int fb_hIsMonth( const char *text, size_t text_len, const char **end_text, int short_name, int localized )
{
    const char *txt_end = text;
    int month;
    for( month=1; month!=13; ++month ) {
        FBSTRING_ACCESS *sMonthName = fb_IntlGetMonthName( month, short_name, !localized );
        DBG_ASSERT( sMonthName!=NULL );
        {
            size_t month_len = FB_STRSIZE( sMonthName );
            size_t len = ((text_len < month_len) ? text_len : month_len );
            int is_same = FB_MEMCMP( text, sMonthName->data, len ) == 0;
            fb_hStrDelTemp( sMonthName );
            if( is_same ) {
                if( text_len > len ) {
                    if( !isalpha( FB_CHAR_TO_INT(text[len]) ) ) {
                        txt_end = text + len;
                        break;
                    }
                } else {
                    txt_end = text + len;
                    break;
                }
            }
        }
    }
    if( month!=13 ) {
        if( short_name ) {
            /* There might follow a dot directly after the
             * abbreviated month name */
            if( txt_end < text + text_len && *txt_end=='.' )
                ++txt_end;
        }
    } else {
        month = 0;
    }
    if( end_text!=NULL )
        *end_text = txt_end;
    return month;
}

/*:::::*/
static int fb_hFindMonth( const char *text, size_t text_len, const char **end_text )
{
    int short_name;
    for( short_name=0; short_name!=2; ++short_name ) {
        int localized = 2;
        while( localized-- ) {
            int month = fb_hIsMonth( text, text_len, end_text, short_name, localized );
            if( month!=0 ) {
                return month;
            }
        }
    }
    return 0;
}

/*:::::*/
static int fb_hDateOrder( int *pOrderDay, int *pOrderMonth, int *pOrderYear )
{
    int order_month = 0, order_day = 1, order_year = 2, order_index = 0;
    int tmp, got_sep;
    char short_format[90];

    tmp = fb_IntlGetDateFormat( short_format, sizeof(short_format), FALSE );
    if( !tmp ) {
        return FALSE;
    }

    got_sep = TRUE;
    for( tmp=0; short_format[tmp]; ++tmp ) {
        int ch = FB_CHAR_TO_INT( short_format[tmp] );
        if( islower(ch) )
            ch = toupper( ch );
        switch ( ch ) {
        case 'D':
            order_day = order_index;
            got_sep = FALSE;
            break;
        case 'M':
            order_month = order_index;
            got_sep = FALSE;
            break;
        case 'Y':
            order_year = order_index;
            got_sep = FALSE;
            break;
        default:
            if( !got_sep ) {
                ++order_index;
            }
            got_sep = TRUE;
            break;
        }
    }

    if( order_day==order_month || order_day==order_year || order_month==order_year )
        return FALSE;
    if( order_day > 2 || order_month > 2 || order_year > 2 )
        return FALSE;

    if( pOrderDay )
        *pOrderDay = order_day;
    if( pOrderMonth )
        *pOrderMonth = order_month;
    if( pOrderYear )
        *pOrderYear = order_year;

    return TRUE;
}

/*:::::*/
static __inline__
int InlineSelect( int index, int num1, int num2, int num3 )
{
    if( index==0 )
        return num1;
    if( index==1 )
        return num2;
    if( index==2 )
        return num3;
    return 0;
}

/*:::::*/
static __inline__
size_t InlineSelectSize( int index, size_t num1, size_t num2, size_t num3 )
{
    if( index==0 )
        return num1;
    if( index==1 )
        return num2;
    if( index==2 )
        return num3;
    return 0;
}

/*:::::*/
int fb_hDateParse( const char *text, size_t text_len,
                   int *pDay, int *pMonth, int *pYear,
                   size_t *pLength )
{
    size_t length = 0, len = text_len;
    const char *text_start = text;
    const char *text_end;
    int result = FALSE;
    int year = 1899, month = 12, day = 30;
    int order_year, order_month, order_day;
    const char *end_month;

    if( text == NULL || text_len == 0 || text_len > (size_t)PTRDIFF_MAX )
        return FALSE;
    text_end = text + text_len;

    if( !fb_hDateOrder( &order_day, &order_month, &order_year ) ) {
        /* switch to US date format */
        order_month = 0;
        order_day = 1;
        order_year = 2;
    }

    /* skip white spaces */
    while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
        ++text;
    len = text_len - (size_t)(text - text_start);

    month = fb_hFindMonth( text, len, &end_month );
    if( month != 0 ) {
        /* The string has the form: (MMMM|MMM) (d|dd)"," (yy|yyyy)  */
        const char *endptr;
        text = end_month;
        if( !fb_hDateNumber( text, text_end, &endptr, &day ) )
            return FALSE;
        if( day>0 ) {

            /* skip white spaces */
            text = endptr;
            while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                ++text;

            if( text < text_end && *text==',' ) {
                size_t year_size;
                ++text;
                if( !fb_hDateNumber( text, text_end, &endptr, &year ) )
                    return FALSE;
                year_size = (size_t)(endptr - text);
                if( year_size > 0 ) {
                    if( year_size==2 )
                        year += 1900;

                    result = day <= fb_hTimeDaysInMonth( month, year );
                    text = endptr;
                }
            }
        }
    } else {
        /* The string can be in the short or long format.
         *
         * The short format can be
         * [0-9]{1,2}(/|\-|\.)[0-9]{1,2}\1[0-9]{1,4}
         *                              ^^ reference to first divider
         *
         * The long format can have the form:
         * (d|dd) (MMMM|MM)"," (yy|yyyy)
         */
        size_t day_size;
        const char *endptr;
        int valid_divider;
        if( !fb_hDateNumber( text, text_end, &endptr, &day ) )
            return FALSE;
        day_size = (size_t)(endptr - text);
        if( day_size ) {
            size_t month_size = 0;
            char chDivider;
            int is_short_form;

            /* skip white spaces */
            text = endptr;
            while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                ++text;

            /* read month and additional dividers */
            if( text == text_end )
                return FALSE;
            chDivider = *text;
            valid_divider = chDivider=='-' || chDivider=='/' || chDivider=='.';
            if( chDivider=='.' ) {
                ++text;
                /* skip white spaces */
                while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                    ++text;
                len = text_len - (size_t)(text - text_start);
                month = fb_hFindMonth( text, len, &end_month );
                /* We found a dot but a month name ... so this date
                 * is in LONG format. */
                is_short_form = month==0;
            } else if( valid_divider ) {
                ++text;
                is_short_form = TRUE;
            } else {
                is_short_form = FALSE;
            }
            if( is_short_form ) {
                /* short date */
                /* skip white spaces */
                while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                    ++text;
                if( !fb_hDateNumber( text, text_end, &endptr, &month ) )
                    return FALSE;
                month_size = (size_t)(endptr - text);
                if( month_size ) {
                    text = endptr;
                    /* skip white spaces */
                    while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                        ++text;
                    if( text < text_end && *text==chDivider ) {
                        ++text;
                        result = TRUE;
                    }
                }
            } else {
                /* long date */
                len = text_len - (size_t)(text - text_start);
                month = fb_hFindMonth( text, len, &end_month );
                if( month != 0 ) {
                    text = end_month;
                    /* skip white spaces */
                    while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                        ++text;
                    /* this comma is optional */
                    if( text < text_end && *text==',' ) {
                        ++text;
                    }
                    result = TRUE;
                }
            }
            /* read year */
            if( result ) {
                size_t year_size;
                /* skip white spaces */
                while ( text < text_end && isspace( FB_CHAR_TO_INT( *text ) ) )
                    ++text;
                if( !fb_hDateNumber( text, text_end, &endptr, &year ) )
                    return FALSE;
                year_size = (size_t)(endptr - text);
                if( year_size > 0 ) {
                    /* adjust short form according to the date format */
                    if( is_short_form ) {
                        int tmp_day = InlineSelect( order_day, day, month, year );
                        int tmp_month = InlineSelect( order_month, day, month, year );
                        int tmp_year = InlineSelect( order_year, day, month, year );
                        year_size = InlineSelectSize( order_year, day_size,
                                                     month_size, year_size );
                        day = tmp_day;
                        month = tmp_month;
                        year = tmp_year;
                        if( day < 1 || month < 1 || month > 12 )
                            result = FALSE;
                    }

                    if( result ) {
                        if( year_size==2 )
                            year += 1900;
                        result = day <= fb_hTimeDaysInMonth( month, year );
                    }
                    text = endptr;
                } else {
                    result = FALSE;
                }
            }
        }
    }

    if( result ) {
        /* Update used length */
        length = (size_t)(text - text_start);
    }

    if( result ) {
        if( pDay )
            *pDay = day;
        if( pMonth )
            *pMonth = month;
        if( pYear )
            *pYear = year;
        if( pLength )
            *pLength = length;
    }

    return result;
}

/* end of datetime/time_parsedate.c */
