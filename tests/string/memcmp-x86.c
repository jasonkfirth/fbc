/*
 * FreeBASIC runtime tests
 * File: memcmp-x86.c
 * Purpose: Compare the x86 runtime helper with the C library's byte ordering.
 * Responsibilities: Exercise alignment, short tails, unsigned bytes and the
 *     first mismatch. Windows guard pages detect reads beyond supplied spans.
 * This file does not link the runtime or measure application performance.
 */
#include "../../src/rtlib/fb.h"
#ifdef HOST_WIN32
#include <windows.h>
#endif

#ifndef HOST_X86
#error This test must be compiled for 32-bit x86.
#endif

static int failures;
static unsigned long comparisons;
static int (*volatile reference_compare)(const void *, const void *, size_t) = memcmp;

static void compare_span(const unsigned char *first, const unsigned char *second,
                         size_t length)
{
    int actual = FB_MEMCMP(first, second, length);
    int expected = reference_compare(first, second, length);
    comparisons++;
    if ((actual > 0) - (actual < 0) != (expected > 0) - (expected < 0)) {
        if (failures < 8)
            printf("FAIL length=%lu actual=%d expected=%d\n",
                   (unsigned long)length, actual, expected);
        failures++;
    }
}

static void test_spans(void)
{
    unsigned char first[80], second[80];
    size_t first_offset, second_offset, length, position;
    unsigned int value;

    /* Independent offsets cover every alignment within a four-byte word. */
    for (first_offset = 0; first_offset < 4; first_offset++) {
        for (second_offset = 0; second_offset < 4; second_offset++) {
            for (length = 0; length <= 65; length++) {
                for (position = 0; position < length; position++)
                    first[first_offset + position] =
                        second[second_offset + position] =
                        (unsigned char)(position * 37 + 129);
                /* Differing bytes outside length must never affect equality. */
                first[first_offset + length] = 0;
                second[second_offset + length] = 255;
                compare_span(first + first_offset, second + second_offset, length);
                for (position = 0; position < length; position++) {
                    unsigned char original = second[second_offset + position];
                    for (value = 0; value <= 255; value++) {
                        second[second_offset + position] = (unsigned char)value;
                        compare_span(first + first_offset, second + second_offset, length);
                    }
                    second[second_offset + position] = original;
                }
            }
        }
    }
    /* Word numeric order opposes the order of the first unequal byte. */
    memset(first, 0, sizeof(first));
    memset(second, 0, sizeof(second));
    first[0] = 1;
    second[3] = 255;
    compare_span(first, second, 4);
    compare_span(second, first, 4);
    if (FB_MEMCMP(NULL, NULL, 0) != 0)
        failures++;
}

#ifdef HOST_WIN32
static void test_guard_pages(void)
{
    SYSTEM_INFO info;
    unsigned char *first, *second;
    DWORD previous;
    size_t length;

    GetSystemInfo(&info);
    first = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE,
                         PAGE_READWRITE);
    second = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);
    if (!first || !second ||
        !VirtualProtect(first + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &previous) ||
        !VirtualProtect(second + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &previous)) {
        failures++;
        goto cleanup;
    }
    memset(first, 129, info.dwPageSize);
    memset(second, 129, info.dwPageSize);
    for (length = 0; length <= 257; length++) {
        unsigned char *left = first + info.dwPageSize - length;
        unsigned char *right = second + info.dwPageSize - length;
        compare_span(left, right, length);
        if (length) {
            right[length - 1] = 255;
            compare_span(left, right, length);
            right[length - 1] = 129;
        }
    }
cleanup:
    if (first) VirtualFree(first, 0, MEM_RELEASE);
    if (second) VirtualFree(second, 0, MEM_RELEASE);
}
#endif

int main(void)
{
    test_spans();
#ifdef HOST_WIN32
    test_guard_pages();
#endif
    printf("x86 memory comparison: %lu comparisons, %d failures\n", comparisons, failures);
    return failures ? 1 : 0;
}
/* end of memcmp-x86.c */
