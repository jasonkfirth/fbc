/*
    Project: FreeBASIC compiler backend regression tests
    ---------------------------------------------------

    File: integer128-gas32-c-abi.c

    Purpose:

        Exercise the 32-bit C aggregate ABI used for FreeBASIC 128-bit scalars.

    Responsibilities:

        - accept and return the aligned two-word C representation by value
        - check the exact low/high words seen by the FreeBASIC caller

    This file intentionally does NOT contain:

        - native __int128, which GCC does not support on i386
        - FreeBASIC runtime or compiler implementation code
*/

#include <stdint.h>

typedef struct __attribute__((aligned(16))) FB_INT128_C_VALUE {
	uint64_t low;
	uint64_t high;
} FB_INT128_C_VALUE;

FB_INT128_C_VALUE c2fb_gas32_roundtrip(FB_INT128_C_VALUE value)
{
	FB_INT128_C_VALUE result;
	result.low = value.low + 1;
	result.high = value.high + (result.low < value.low);
	return result;
}

int64_t c2fb_gas32_matches(FB_INT128_C_VALUE value,
					   uint64_t expected_low, uint64_t expected_high)
{
	return value.low == expected_low && value.high == expected_high;
}

/* end of integer128-gas32-c-abi.c */
