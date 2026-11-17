/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/date-parse-runtime.c
 * Purpose: Check public date parsing through the actual runtime archive.
 * Responsibilities: Preserve ordinary dates and reject overflowing numeric fields.
 * Native serial I/O and locale configuration have their own test fixtures.
 */
#include "fb.h"

static int checks;

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "DATE_PARSE_RUNTIME_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

static void parse( const char *text, int valid, int day, int month, int year )
{
    FBSTRING input;
    char value[128];
    size_t length = strlen(text);
    int actual_day = 123, actual_month = 124, actual_year = 125;
    int result;
    check(length < sizeof(value));
    memcpy(value, text, length + 1);
    input.data = value;
    input.len = (ssize_t)length;
    input.size = input.len;
    result = fb_DateParse(&input, &actual_day, &actual_month, &actual_year);
    check((result != FALSE) == valid);
    check((fb_IsDate(&input) != FALSE) == valid);
    if( valid ) {
        check(actual_day == day && actual_month == month && actual_year == year);
        check(fb_DateValue(&input) == fb_DateSerial(year, month, day));
    } else {
        check(fb_DateValue(&input) == 0);
        check(fb_ErrorGetNum() == FB_RTERROR_ILLEGALFUNCTIONCALL);
    }
}

int main( int argc, char **argv )
{
    fb_Init(argc, argv, 0);
    fb_I18nSet(FALSE);
    parse("Aug 5, 2005", TRUE, 5, 8, 2005);
    parse("5 Aug. 2005", TRUE, 5, 8, 2005);
    parse("8/5/2005", TRUE, 5, 8, 2005);
    parse("2/29/2000", TRUE, 29, 2, 2000);
    parse("2/29/1900", FALSE, 0, 0, 0);
    parse("1/1/4294969322", FALSE, 0, 0, 0);
    parse("4294967297/1/2026", FALSE, 0, 0, 0);
    parse("1/4294967297/2026", FALSE, 0, 0, 0);
    parse("January 4294967297, 2026", FALSE, 0, 0, 0);
    parse("January 1, 4294969322", FALSE, 0, 0, 0);
    parse("1/1/999999999999999999999999999999", FALSE, 0, 0, 0);
    printf("DATE_PARSE_RUNTIME_PASS checks=%d\n", checks);
    return 0;
}

/* end of rtlib/date-parse-runtime.c */
