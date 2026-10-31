/*
    Project: FreeBASIC C backend regression controls
    File: c-conversion-results-guards.c
    Purpose: Check the actual emitted native 128-bit conversion guards.
    Responsibilities: Valid boundary values, rejected inputs and four rounding
    modes under float-cast instrumentation. The generated header is copied
    verbatim from the compiler output by the qualification harness.
    This file intentionally does NOT contain: Conversion implementations or
    execution of libc abort. A rejected input returns through this test's jump.
*/
#include <fenv.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

static jmp_buf rejected_input;

static void reject_conversion(void)
{
    longjmp(rejected_input, 1);
}

/* Keep the production guard and intercept only its terminal failure action. */
#define __builtin_abort reject_conversion
#include "conversion-helpers.h"
#undef __builtin_abort

static int check_double(double value, int is_unsigned, int reject, __uint128_t expected)
{
    __uint128_t observed;
    if (setjmp(rejected_input) != 0) return reject ? 0 : 1;
    if (is_unsigned) {
        observed = fb_D2U128(value);
    } else {
        const __int128_t result = fb_D2I128(value);
        memcpy(&observed, &result, sizeof(observed));
    }
    return reject || observed != expected;
}

static int check_single(float value, int is_unsigned, int reject, __uint128_t expected)
{
    __uint128_t observed;
    if (setjmp(rejected_input) != 0) return reject ? 0 : 1;
    if (is_unsigned) {
        observed = fb_F2U128(value);
    } else {
        const __int128_t result = fb_F2I128(value);
        memcpy(&observed, &result, sizeof(observed));
    }
    return reject || observed != expected;
}

int main(void)
{
    const int modes[] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };
    /* IEEE infinity, quiet/signaling NaNs, positive limit and negative overflow. */
    const uint64_t signed_double[] = {
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
        UINT64_C(0x47e0000000000000), UINT64_C(0xc7e0000000000001)
    };
    const uint64_t unsigned_double[] = {
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000042), UINT64_C(0x7ff0000000000001),
        UINT64_C(0x47f0000000000000), UINT64_C(0xbff0000000000000)
    };
    const uint32_t signed_single[] = {
        UINT32_C(0x7f800000), UINT32_C(0xff800000),
        UINT32_C(0x7fc00042), UINT32_C(0x7f800001),
        UINT32_C(0x7f000000), UINT32_C(0xff000001)
    };
    const uint32_t unsigned_single[] = {
        UINT32_C(0x7f800000), UINT32_C(0xff800000),
        UINT32_C(0x7fc00042), UINT32_C(0x7f800001), UINT32_C(0xbf800000)
    };
    const __uint128_t one = 1;
    const __uint128_t sign_bit = one << 127;
    const __uint128_t all_bits = ~(__uint128_t)0;
    unsigned int mode;
    unsigned int index;

    for (mode = 0; mode < sizeof(modes) / sizeof(modes[0]); ++mode) {
        if (fesetround(modes[mode]) != 0) return 1;
        if (check_double(-0x1p+127, 0, 0, sign_bit)) return 2;
        if (check_double(0x1.fffffffffffffp+126, 0, 0, sign_bit - (one << 74))) return 3;
        if (check_double(0x1.fffffffffffffp+127, 1, 0, all_bits - ((one << 75) - 1))) return 4;
        if (check_single(-0x1p+127f, 0, 0, sign_bit)) return 5;
        if (check_single(0x1.fffffep+126f, 0, 0, sign_bit - (one << 103))) return 6;
        if (check_single(0x1.fffffep+127f, 1, 0, all_bits - ((one << 104) - 1))) return 7;

        for (index = 0; index < sizeof(signed_double) / sizeof(signed_double[0]); ++index) {
            double value;
            memcpy(&value, &signed_double[index], sizeof(value));
            if (check_double(value, 0, 1, 0)) return 8;
            memcpy(&value, &unsigned_double[index], sizeof(value));
            if (check_double(value, 1, 1, 0)) return 9;
        }
        for (index = 0; index < sizeof(signed_single) / sizeof(signed_single[0]); ++index) {
            float value;
            memcpy(&value, &signed_single[index], sizeof(value));
            if (check_single(value, 0, 1, 0)) return 10;
        }
        for (index = 0; index < sizeof(unsigned_single) / sizeof(unsigned_single[0]); ++index) {
            float value;
            memcpy(&value, &unsigned_single[index], sizeof(value));
            if (check_single(value, 1, 1, 0)) return 11;
        }
    }
    if (fesetround(FE_TONEAREST) != 0) return 12;
    return 0;
}

/* end of c-conversion-results-guards.c */
