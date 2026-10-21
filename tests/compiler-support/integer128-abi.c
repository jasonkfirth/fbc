/*
    Project: FreeBASIC compiler backend regression tests
    -----------------------------------------

    File: integer128-abi.c

    Purpose:

        Provide external C functions for the FreeBASIC 128-bit ABI tests.

    Responsibilities:

        - receive and return signed and unsigned 128-bit integers

    This file intentionally does NOT contain:

        - the FreeBASIC test harness or compiler backend implementation
*/

__int128 c2fb_test_add_signed128(__int128 left, __int128 right)
{
	return left + right;
}

unsigned __int128 c2fb_test_add_unsigned128(unsigned __int128 left,
										   unsigned __int128 right)
{
	return left + right;
}

/* end of integer128-abi.c */
