/*
    Project: FreeBASIC C backend regression controls
    File: c-native-int128-caller.c
    Purpose: Check the native C ABI against separately compiled BASIC callees.
    Responsibilities: Pass and return signed/unsigned values above 64 bits.
    This file intentionally does NOT contain: Platform APIs or BASIC runtime code.
*/

extern __int128_t fbc_test_signed128(__int128_t value);
extern __uint128_t fbc_test_unsigned128(__uint128_t value);
int fbc_test_native128_caller(void);

int fbc_test_native128_caller(void)
{
    __int128_t signed_value = -((__int128_t)1 << 100) + 41;
    __uint128_t unsigned_value = ((__uint128_t)1 << 100) | 37;

    int failed = 0;
    if (fbc_test_signed128(signed_value) != signed_value + 1)
        failed |= 1;
    if (fbc_test_unsigned128(unsigned_value) != ~unsigned_value)
        failed |= 2;
    return failed;
}

/* end of c-native-int128-caller.c */
