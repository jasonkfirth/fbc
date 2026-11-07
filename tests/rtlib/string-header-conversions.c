/*
 * Project: FreeBASIC runtime qualification
 * File: string-header-conversions.c
 * Purpose: Exercise the production string header helpers at their boundaries.
 * Responsibilities: Check allocation limits, packed wide strings and descriptor
 *     ownership flags for each supported character width and native word size.
 * This file intentionally does NOT allocate large buffers or model a platform ABI.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if FB_TEST_WCHAR_WIDTH == 1
typedef uint8_t FB_WCHAR;
#elif FB_TEST_WCHAR_WIDTH == 2
typedef uint16_t FB_WCHAR;
#elif FB_TEST_WCHAR_WIDTH == 4
typedef uint32_t FB_WCHAR;
#else
#error Unsupported character width
#endif

#if UINTPTR_MAX > UINT32_MAX
#define HOST_64BIT
#endif
#define TRUE 1
#define FB_WCHAR_LITERAL(c) c

static size_t allocation_calls, allocation_bytes, compare_calls;
static union {
    max_align_t alignment;
    unsigned char bytes[128];
} allocation_buffer;

/* Record the requested size, including impossible requests, without reserving
 * any corresponding memory. This makes the upper-bound checks inexpensive. */
static void *test_allocate(size_t bytes)
{
    ++allocation_calls;
    allocation_bytes = bytes;
    return bytes <= sizeof(allocation_buffer.bytes) ? allocation_buffer.bytes : NULL;
}

static size_t test_wcslen(const FB_WCHAR *text)
{
    size_t length = 0;
    while (text[length] != 0)
        ++length;
    return length;
}

static int test_wcsncmp(const FB_WCHAR *left, const FB_WCHAR *right, size_t count)
{
    ++compare_calls;
    for (size_t i = 0; i < count; ++i) {
        if (left[i] != right[i])
            return left[i] < right[i] ? -1 : 1;
        if (left[i] == 0)
            break;
    }
    return 0;
}

/* FB_MEMCPYX returns the byte after the copy, unlike standard memcpy. The
 * fixtures use distinct source/destination buffers, as the real helper requires. */
static void *test_memcpyx(void *destination, const void *source, size_t count)
{
    memcpy(destination, source, count);
    return (unsigned char *)destination + count;
}

#define malloc test_allocate
#define wcslen test_wcslen
#define wcsncmp test_wcsncmp
#define FB_MEMCPYX test_memcpyx
#include "string-header-fragments.h"
#undef malloc
#undef wcslen
#undef wcsncmp

#define CHECK(condition) do { if (!(condition)) { \
    printf("string header failure at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    FB_WCHAR source[] = {65, 66, 67, 0};
    FB_WCHAR other[] = {65, 66, 68, 0};
    FB_WCHAR destination[8] = {0};
    union {
        max_align_t alignment;
        unsigned char bytes[64];
    } packed_source, packed_destination, packed_descriptor;
    const ssize_t maximum = (ssize_t)((size_t)PTRDIFF_MAX / sizeof(FB_WCHAR) - 1);

    /* Keep this first: the old helper fails here without reaching its signed
     * overflow at the positive limit. The control run therefore has no UB. */
    CHECK(fb_wstr_AllocTemp(-1) == NULL);
    CHECK(allocation_calls == 0);
    for (ssize_t count = 0; count <= 7; ++count) {
        CHECK(fb_wstr_AllocTemp(count) != NULL);
        CHECK(allocation_bytes == ((size_t)count + 1) * sizeof(FB_WCHAR));
    }
    allocation_calls = 0;
    CHECK(fb_wstr_AllocTemp(maximum) == NULL);
    CHECK(allocation_calls == 1);
    CHECK(allocation_bytes == ((size_t)maximum + 1) * sizeof(FB_WCHAR));
    allocation_calls = 0;
    CHECK(fb_wstr_AllocTemp(maximum + 1) == NULL);
    CHECK(fb_wstr_AllocTemp(PTRDIFF_MAX) == NULL);
    CHECK(allocation_calls == 0);

    CHECK(fb_wstr_Len(source) == 3);
    fb_wstr_Copy(destination, source, 3);
    CHECK(memcmp(destination, source, sizeof(source)) == 0);
    CHECK(fb_wstr_Move(destination, other, 3) == destination + 3);
    CHECK(memcmp(destination, other, 3 * sizeof(FB_WCHAR)) == 0);
    CHECK(fb_wstr_Move(destination, NULL, 0) == destination);
    CHECK(fb_wstr_Move(destination, NULL, -1) == destination);
    fb_wstr_Copy(destination, NULL, -1);
    CHECK(destination[0] == 0);
    compare_calls = 0;
    CHECK(fb_wstr_Compare(NULL, NULL, 0) == 0);
    CHECK(fb_wstr_Compare(NULL, NULL, -1) == 0);
    CHECK(compare_calls == 0);
    CHECK(fb_wstr_Compare(source, other, 2) == 0);
    CHECK(fb_wstr_Compare(source, other, 3) < 0);
    CHECK(fb_wstr_Compare(other, source, 3) > 0);
    CHECK(fb_wstr_Compare(source, source, 4) == 0);

    /* One byte after aligned storage forces the production byte-copy paths for
     * wide characters. Width one still checks the same values and terminator. */
    memcpy(packed_source.bytes + 1, source, sizeof(source));
    const FB_WCHAR *packed = (const FB_WCHAR *)(const void *)(packed_source.bytes + 1);
    FB_WCHAR *packed_output = (FB_WCHAR *)(void *)(packed_destination.bytes + 1);
    CHECK(fb_wstr_Len(packed) == 3);
    CHECK(fb_wstr_Compare(packed, other, 3) < 0);
    CHECK(fb_wstr_Compare(other, packed, 3) > 0);
    fb_wstr_Copy(packed_output, source, 3);
    CHECK(memcmp(packed_destination.bytes + 1, source, sizeof(source)) == 0);
    CHECK((void *)fb_wstr_Move(packed_output, other, 3) ==
          (void *)(packed_destination.bytes + 1 + 3 * sizeof(FB_WCHAR)));
    CHECK(memcmp(packed_destination.bytes + 1, other, 3 * sizeof(FB_WCHAR)) == 0);

    const size_t lengths[] = {0, 1, 17, (size_t)PTRDIFF_MAX};
    for (int temporary = 0; temporary <= 1; ++temporary) {
        for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
            FBSTRING initial = {(char *)source, temporary ? FB_TEMPSTRBIT | 3 : 3, 37};
            memcpy(packed_descriptor.bytes + 1, &initial, sizeof(initial));
            FBSTRING_ACCESS *descriptor = (FBSTRING_ACCESS *)(void *)(packed_descriptor.bytes + 1);
            fb_hStrSetLength(descriptor, lengths[i]);
            CHECK((size_t)FB_STRSIZE(descriptor) == lengths[i]);
            CHECK(FB_ISTEMP(descriptor) == temporary);
            CHECK(descriptor->data == initial.data);
            CHECK(descriptor->size == initial.size);
        }
    }
    puts("STRING_HEADER_CONVERSIONS_PASS");
    return 0;
}

/* end of string-header-conversions.c */
