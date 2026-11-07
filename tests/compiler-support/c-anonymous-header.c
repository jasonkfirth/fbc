/*
 * Project: FreeBASIC C backend regression tests
 * File: c-anonymous-header.c
 * Purpose: Supply an ordinary C declaration for the BASIC anonymous record.
 * Responsibilities: Verify argument and result values without named padding.
 * This file intentionally does NOT include generated C record definitions.
 */
#include <stdint.h>

#ifndef FB_TEST_RECORD
#error FB_TEST_RECORD must name the emitted BASIC record tag
#endif

struct FB_TEST_RECORD {
    uint8_t PREFIX;
    union {
        struct {
            uint8_t FIRST_BYTE;
            uint16_t SMALL_VALUE;
        } __attribute__((packed, aligned(1)));
        uint8_t RAW_VALUES[3];
    } __attribute__((aligned(2)));
    uint8_t TRAILING[2];
};

int32_t fb_test_header_accept( struct FB_TEST_RECORD state );
struct FB_TEST_RECORD fb_test_header_make( void );

int32_t fb_test_header_accept( struct FB_TEST_RECORD state )
{
    return state.PREFIX == 7 && state.FIRST_BYTE == 11 &&
           state.SMALL_VALUE == 0x1234 &&
           state.TRAILING[0] == 13 && state.TRAILING[1] == 17;
}

struct FB_TEST_RECORD fb_test_header_make( void )
{
    struct FB_TEST_RECORD state = {
        .PREFIX = 7, .FIRST_BYTE = 11, .SMALL_VALUE = 0x1234,
        .TRAILING = {13, 17}
    };
    return state;
}

/* end of c-anonymous-header.c */
