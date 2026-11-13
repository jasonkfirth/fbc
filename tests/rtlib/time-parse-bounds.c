/*
 * Project: FreeBASIC runtime tests
 * File: time-parse-bounds.c
 * Purpose: Check time parsing within an explicit byte span.
 * Responsibilities: Preserve valid time syntax and reject truncated fields.
 * This file does not test calendar parsing or change the process locale.
 */
#ifndef FB_TIME_PARSE_SOURCE
#define FB_TIME_PARSE_SOURCE "../../src/rtlib/datetime/time_parsetime.c"
#endif
#include FB_TIME_PARSE_SOURCE

#ifdef _WIN32
#include <windows.h>
#endif

static int checks;

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "TIME_PARSE_BOUNDS_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

static void parse( const char *text, size_t length, int valid,
                  int hour, int minute, int second, size_t consumed )
{
    int actual_hour = 123, actual_minute = 124, actual_second = 125;
    size_t actual_length = 126;
    int result = fb_hTimeParse( text, length, &actual_hour, &actual_minute,
                               &actual_second, &actual_length );
    check( (result != FALSE) == valid );
    if( valid ) {
        check( actual_hour == hour && actual_minute == minute && actual_second == second );
        check( actual_length == consumed && consumed <= length );
    } else {
        check( actual_hour == 123 && actual_minute == 124 && actual_second == 125 && actual_length == 126 );
    }
}

static void prefix( const char *text, size_t length )
{
    /* The suffix is physically readable but outside the supplied span. */
    parse( text, length, TRUE, 12, 34, 0, 5 );
}

int main( void )
{
    char text[16];
    int hour, minute, second;

    prefix( "12:34:56", 5 );
    parse( "12:34:56", 6, FALSE, 0, 0, 0, 0 );
    parse( "12:34:56", 7, TRUE, 12, 34, 5, 7 );
    parse( "12:34:56", 8, TRUE, 12, 34, 56, 8 );
    parse( "12:34:56 pm", 9, TRUE, 12, 34, 56, 8 );
    parse( "12:34:56 pm", 10, TRUE, 12, 34, 56, 10 );
    parse( "12:34:56 pm", 11, TRUE, 12, 34, 56, 11 );
    parse( "12:34\0:56", 9, TRUE, 12, 34, 0, 5 );
    parse( "", 0, FALSE, 0, 0, 0, 0 );
    parse( NULL, 0, FALSE, 0, 0, 0, 0 );
    parse( NULL, 8, FALSE, 0, 0, 0, 0 );
    parse( "1", SIZE_MAX, FALSE, 0, 0, 0, 0 );
    parse( "24:00", 5, FALSE, 0, 0, 0, 0 );
    parse( "12:60", 5, FALSE, 0, 0, 0, 0 );
    parse( "12:00:60", 8, FALSE, 0, 0, 0, 0 );
    parse( "4294967296:00", 13, FALSE, 0, 0, 0, 0 );
    parse( "999999999999999999999999999:00", 30, FALSE, 0, 0, 0, 0 );
    parse( "-0:00", 5, TRUE, 0, 0, 0, 5 );
    parse( "12 A", 4, TRUE, 0, 0, 0, 4 );
    parse( "12 pm", 5, TRUE, 12, 0, 0, 5 );
    parse( "1 am", 4, TRUE, 1, 0, 0, 4 );
    parse( "1 pm", 4, TRUE, 13, 0, 0, 4 );
    parse( "12 ax", 5, FALSE, 0, 0, 0, 0 );
    parse( "12 am!", 6, TRUE, 0, 0, 0, 5 );
    parse( "12:34 ax", 8, TRUE, 12, 34, 0, 6 );
    for( hour = 0; hour < 24; ++hour ) {
        for( minute = 0; minute < 60; ++minute ) {
            for( second = 0; second < 60; ++second ) {
                snprintf( text, sizeof( text ), "%02d:%02d:%02d", hour, minute, second );
                parse( text, 8, TRUE, hour, minute, second, 8 );
            }
        }
    }
#ifdef _WIN32
    {
        SYSTEM_INFO information;
        unsigned char *pages;
        char *last;
        DWORD previous;
        GetSystemInfo( &information );
        pages = VirtualAlloc( NULL, (size_t)information.dwPageSize * 2,
                              MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
        check( pages != NULL );
        check( VirtualProtect( pages + information.dwPageSize,
                               information.dwPageSize, PAGE_NOACCESS, &previous ) != 0 );
        last = (char *)pages + information.dwPageSize;
        memcpy( last - 5, "12:34", 5 );
        prefix( last - 5, 5 );
        memcpy( last - 4, "12 a", 4 );
        parse( last - 4, 4, TRUE, 0, 0, 0, 4 );
        check( VirtualFree( pages, 0, MEM_RELEASE ) != 0 );
    }
#endif
    printf( "TIME_PARSE_BOUNDS_PASS checks=%d\n", checks );
    return 0;
}

/* end of time-parse-bounds.c */
