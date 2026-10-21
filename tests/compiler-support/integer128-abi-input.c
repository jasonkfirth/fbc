/*
    Project: FreeBASIC compiler backend regression tests
    ----------------------------------------------------

    File: integer128-abi-input.c

    Purpose:

        Provide C functions that verify 128-bit integer arguments passed by
        the FreeBASIC native GAS64 backend.

    Responsibilities:

        - check signed and unsigned 128-bit values received in the SysV ABI

    This file intentionally does NOT contain:

        - FreeBASIC compiler implementation code
*/

#include <stdint.h>

int64_t c2fb_test_int128_args(__int128 signed_value,
			      unsigned __int128 unsigned_value,
			      unsigned __int128 third_value,
			      __int128 fourth_value)
{
	if (signed_value != -8) return 0;
	if ((unsigned_value >> 100) != 1) return 0;
	if (third_value != 41) return 0;
	if (fourth_value != -9) return 0;
	return 42;
}

/* end of integer128-abi-input.c */
