/*
 * Project: FreeBASIC runtime tests
 * File: time-parse-runtime.c
 * Purpose: Check the public time parser through the actual runtime archive.
 * Responsibilities: Exercise ordinary fields, AM/PM and invalid numeric input.
 * This file does not test locale formatting or construct calendar dates.
 */
#include "fb.h"

static int checks;

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "TIME_PARSE_RUNTIME_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

static void parse( char *text, int valid, int hour, int minute, int second )
{
    FBSTRING input;
    int h = -1, m = -1, s = -1;
    int result;
    input.data = text;
    input.len = (ssize_t)strlen( text );
    input.size = input.len;
    result = fb_TimeParse( &input, &h, &m, &s );
    check( (result != FALSE) == valid );
    if( valid ) check( h == hour && m == minute && s == second );
}

int main( int argc, char **argv )
{
    char text[32];
    const int parts[] = {0, 1, 59};
    int h;
    size_t m, s;
    fb_Init( argc, argv, 0 );
    for( h = 0; h < 24; ++h ) {
        for( m = 0; m < sizeof( parts ) / sizeof( parts[0] ); ++m ) {
            for( s = 0; s < sizeof( parts ) / sizeof( parts[0] ); ++s ) {
                snprintf( text, sizeof( text ), "%02d:%02d:%02d", h, parts[m], parts[s] );
                parse( text, TRUE, h, parts[m], parts[s] );
            }
        }
    }
    parse( "12 am", TRUE, 0, 0, 0 );
    parse( "12 pm", TRUE, 12, 0, 0 );
    parse( "1 pm", TRUE, 13, 0, 0 );
    parse( "  +12 : +34 : -00 PM ", TRUE, 12, 34, 0 );
    parse( "24:00", FALSE, 0, 0, 0 );
    parse( "12:60", FALSE, 0, 0, 0 );
    parse( "12:00:60", FALSE, 0, 0, 0 );
    parse( "4294967296:00", FALSE, 0, 0, 0 );
    printf( "TIME_PARSE_RUNTIME_PASS checks=%d\n", checks );
    return 0;
}

/* end of time-parse-runtime.c */
