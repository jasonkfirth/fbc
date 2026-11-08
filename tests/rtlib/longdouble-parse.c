/*
 * Project: FreeBASIC runtime regression tests
 * File: longdouble-parse.c
 * Purpose: Check the Haiku long-double parser independently of its narrow CRT.
 * Responsibilities: Exact rounding, ranges, special values and end positions.
 * This file intentionally does NOT change process locales or allocate a heap.
 */
#include "../../src/rtlib/math/longdouble_parse.h"
#include <stdio.h>
#include <stdlib.h>

static int check(const char *text, long double expected, size_t length)
{
    size_t consumed;
    long double value = fb_hParseLongDouble(text, &consumed);
    if (consumed != length || value != expected ||
        (value == 0 && !!signbit(value) != !!signbit(expected))) {
        printf("long-double parse failed: %s got=%La expected=%La consumed=%zu/%zu\n",
               text, value, expected, consumed, length);
        return 0;
    }
    return 1;
}

int main(void)
{
    static const struct { const char *text; long double value; size_t length; } cases[] = {
        {"", 0.0L, 0}, {" \t+", 0.0L, 0}, {"in", 0.0L, 0},
        {"infinite", INFINITY, 3}, {"0x.p", 0.0L, 1},
        {"0", 0.0L, 1}, {"-0", -0.0L, 2}, {"  +1.75x", 1.75L, 7},
        {"1e+", 1.0L, 1}, {"0x", 0.0L, 1}, {".", 0.0L, 0},
        {"0x1.8p+2", 6.0L, 8}, {"inf", INFINITY, 3}, {"-infinity", -INFINITY, 9},
        {"1.0000000000000001", 1.0000000000000001L, 18},
#if LDBL_MANT_DIG == 64
        {"1.000000000000000000108420217248550443400745280086994171142578125",
         0x1.0000000000000002p0L, 65},
        {"0x1p-16445", 0x1p-16445L, 10},
        {"0x1.fffffffffffffffep16383", 0x1.fffffffffffffffep16383L, 26},
#endif
    };
    size_t i, consumed;
    char long_significand[20003];
    unsigned int checks = 0;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (!check(cases[i].text, cases[i].value, cases[i].length)) return 1;
        ++checks;
    }
    if (!isnan(fb_hParseLongDouble("nan(payload)", &consumed)) || consumed != 12) return 2;
    if (!isnan(fb_hParseLongDouble("nan(", &consumed)) || consumed != 3) return 2;
    long_significand[0] = '1';
    long_significand[1] = '.';
    memset(long_significand + 2, '1', sizeof(long_significand) - 3);
    long_significand[sizeof(long_significand) - 1] = '\0';
    if (!check(long_significand, 1.1111111111111111111111111111111111111111L,
               sizeof(long_significand) - 1)) return 4;
    checks += 3;
#ifndef __HAIKU__
    /* Host CRT comparison is useful only where strtold retains native precision.
     * The fixed vectors above remain authoritative on Haiku itself. */
    for (i = 0; i < 12000; ++i) {
        char text[96], *end;
        long double expected;
        int exponent = (int)(i % 9900) - 4950;
        (void)snprintf(text, sizeof(text), "%u.%09ue%d", (unsigned int)(i * 7919u),
                       (unsigned int)((i * 104729u) % 1000000000u), exponent);
        expected = strtold(text, &end);
        if (!check(text, expected, (size_t)(end - text))) return 3;
        ++checks;
    }
#endif
    printf("LONGDOUBLE_PARSE_PASS checks=%u\n", checks);
    return 0;
}

/* end of longdouble-parse.c */
