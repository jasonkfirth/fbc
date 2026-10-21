/*
    Project: FreeBASIC compiler backend regression tests
    ---------------------------------------------------

    File: integer128-gas32-c-abi-main.c

    Purpose:

        Call an exported GAS32 FreeBASIC procedure using the GCC i386
        aggregate ABI.

    Responsibilities:

        - provide the process entry point and fb_End runtime hook
        - pass and receive the aligned two-word value by value

    This file intentionally does NOT contain:

        - 128-bit arithmetic implementation
        - assumptions about native i386 __int128 support
*/

#include <stdint.h>
#include <stdlib.h>

typedef struct __attribute__((aligned(16))) FB_INT128_C_VALUE {
	uint64_t low;
	uint64_t high;
} FB_INT128_C_VALUE;

extern FB_INT128_C_VALUE c2fb_gas32_fbc_roundtrip(FB_INT128_C_VALUE value);

void fb_End(int exit_code)
{
	exit(exit_code);
}

int main(void)
{
	FB_INT128_C_VALUE value = { 0, 7 };
	FB_INT128_C_VALUE result = c2fb_gas32_fbc_roundtrip(value);
	if (result.low != 1 || result.high != 7) return 1;
	return 0;
}

/* end of integer128-gas32-c-abi-main.c */
