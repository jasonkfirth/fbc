/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/date-parse-bounds.c
 * Purpose: Check date fields within the supplied byte span and C-int range.
 * Responsibilities: Exercise date orders, calendar values, numeric limits and
 *     readable suffixes outside the requested span.
 * Locale names and date-order configuration belong to the observers below.
 */
#ifndef FB_DATE_PARSE_SOURCE
#define FB_DATE_PARSE_SOURCE "../../src/rtlib/datetime/time_parsedate.c"
#endif
#include FB_DATE_PARSE_SOURCE
#include <limits.h>

static int checks, date_order;
static char months[12][10] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "DATE_PARSE_BOUNDS_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

int fb_IntlGetDateFormat( char *buffer, size_t length, int disallow_localized )
{
    static const char *const formats[] = {
        "MM/dd/yyyy", "MM/yyyy/dd", "dd/MM/yyyy",
        "dd/yyyy/MM", "yyyy/MM/dd", "yyyy/dd/MM"
    };
    size_t size = strlen(formats[date_order]) + 1;
    (void)disallow_localized;
    check( size <= length );
    memcpy( buffer, formats[date_order], size );
    return TRUE;
}

FBSTRING_ACCESS *fb_IntlGetMonthName( int month, int short_name, int disallow_localized )
{
    static FBSTRING_ACCESS result;
    (void)disallow_localized;
    result.data = months[month - 1];
    result.len = short_name ? 3 : (ssize_t)strlen(result.data);
    result.size = result.len;
    return &result;
}

FBCALL int fb_hStrDelTemp( FBSTRING_ACCESS *text )
{
    (void)text;
    return 0;
}

int fb_hTimeDaysInMonth( int month, int year )
{
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int leap = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    check( month >= 1 && month <= 12 );
    return days[month - 1] + (month == 2 && leap);
}

static void parse( const char *text, size_t length, int valid,
                   int day, int month, int year, size_t consumed )
{
    int actual_day = 123, actual_month = 124, actual_year = 125;
    size_t actual_length = 126;
    int result = fb_hDateParse(text, length, &actual_day, &actual_month, &actual_year, &actual_length);
    check( (result != FALSE) == valid );
    if( valid ) {
        check( actual_day == day && actual_month == month && actual_year == year );
        check( actual_length == consumed && consumed <= length );
    } else {
        check( actual_day == 123 && actual_month == 124 && actual_year == 125 && actual_length == 126 );
    }
}

int main( void )
{
    static const int years[] = {0, 1, 1900, 2000, 2026};
    static const int order[6][3] = {{1,0,2},{1,2,0},{0,1,2},{0,2,1},{2,1,0},{2,0,1}};
    char text[96];
    int day, month, parts[3];
    size_t index;
    for( date_order = 0; date_order < 6; ++date_order ) {
        for( index = 0; index < sizeof(years) / sizeof(years[0]); ++index ) {
            for( month = 1; month <= 12; ++month ) {
                for( day = 1; day <= 31; ++day ) {
                    parts[0] = day;
                    parts[1] = month;
                    parts[2] = years[index];
                    snprintf( text, sizeof(text), "%d/%d/%d", parts[order[date_order][0]],
                        parts[order[date_order][1]], parts[order[date_order][2]] );
                    parse( text, strlen(text), day <= fb_hTimeDaysInMonth(month, years[index]),
                        day, month, years[index], strlen(text) );
                }
            }
        }
    }
    date_order = 0;
    parse("1/1/4294969322", 14, FALSE, 0, 0, 0, 0);
    parse("4294967297/1/2026", 17, FALSE, 0, 0, 0, 0);
    parse("1/4294967297/2026", 17, FALSE, 0, 0, 0, 0);
    parse("January 4294967297, 2026", 24, FALSE, 0, 0, 0, 0);
    parse("January 1, 4294969322", 21, FALSE, 0, 0, 0, 0);
    parse("1/1/999999999999999999999999999999", 34, FALSE, 0, 0, 0, 0);
    parse("8/5/2005suffix", 8, TRUE, 5, 8, 2005, 8);
    parse("8/5/2005", 6, TRUE, 5, 8, 1920, 6);
    parse("8/5/2005", 4, FALSE, 0, 0, 0, 0);
    parse("Aug 5, 2005", 11, TRUE, 5, 8, 2005, 11);
    parse("5 Aug. 2005", 11, TRUE, 5, 8, 2005, 11);
    parse("Aug", 3, FALSE, 0, 0, 0, 0);
    parse("", 0, FALSE, 0, 0, 0, 0);
    parse(NULL, 0, FALSE, 0, 0, 0, 0);
    parse(NULL, 8, FALSE, 0, 0, 0, 0);
    parse("1", SIZE_MAX, FALSE, 0, 0, 0, 0);
    snprintf(text, sizeof(text), "1/1/%d", INT_MAX);
    parse(text, strlen(text), TRUE, 1, 1, INT_MAX, strlen(text));
    snprintf(text, sizeof(text), "1/1/%d", INT_MIN);
    parse(text, strlen(text), TRUE, 1, 1, INT_MIN, strlen(text));
    printf( "DATE_PARSE_BOUNDS_PASS checks=%d\n", checks );
    return 0;
}

/* end of rtlib/date-parse-bounds.c */
