/*
    Project: FreeBASIC NuttX examples
    ---------------------------------

    File: fblog10_smoke.c

    Purpose:

        Prove that the NuttX smoke runtime can use the normal FreeBASIC
        integer base-10 helper implementation.

    Responsibilities:

        - exercise fb_IntLog10_32()
        - exercise fb_IntLog10_64()
        - keep the result visible in QEMU logs for the device lab

    This file intentionally does NOT contain:

        - floating point logarithm tests
        - string formatting tests
        - board-specific hardware behavior
*/

#include <stdint.h>
#include <stdio.h>

typedef signed int int32;

int fb_IntLog10_32(unsigned int x);
int fb_IntLog10_64(unsigned long long x);

int main(int32 argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (fb_IntLog10_32(0u) != -1)
        return 10;

    if (fb_IntLog10_32(1u) != 0)
        return 11;

    if (fb_IntLog10_32(9u) != 0)
        return 12;

    if (fb_IntLog10_32(10u) != 1)
        return 13;

    if (fb_IntLog10_32(999999999u) != 8)
        return 14;

    if (fb_IntLog10_32(1000000000u) != 9)
        return 15;

    if (fb_IntLog10_64(9999999999ull) != 9)
        return 20;

    if (fb_IntLog10_64(10000000000ull) != 10)
        return 21;

    if (fb_IntLog10_64(9999999999999999999ull) != 18)
        return 22;

    if (fb_IntLog10_64(10000000000000000000ull) != 19)
        return 23;

    printf("FB_NUTTX_LOG10_SMOKE_OK\n");

    return 0;
}

/* end of fblog10_smoke.c */
