/*
    Project: FreeBASIC graphics text regression
    File: wide-glyphs.c
    Purpose: Execute the production UTF-16 glyph conversion on native Windows.
    Responsibilities: Check scalar decoding, allocation boundaries and lock order.
    Rendering and the runtime allocator retain their own integration tests.
*/
#include "fb.h"
#include "gfx_unicode.h"
#include <limits.h>

_Static_assert(sizeof(FB_WCHAR) == 2, "This regression requires native UTF-16 wchar_t");
static FBSTRING descriptor;
static unsigned char storage[64];
static unsigned long long cases, checks, allocations, locks, unlocks;
static ssize_t requested;
static int lock_depth, fail_allocation, last_error;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "GLYPH_UTF16_FAIL case=%llu check=%llu line=%d\n", cases, checks, line);
        exit(1);
    }
}
#define CHECK(condition) require_at((condition), __LINE__)

#ifdef ENABLE_MT
FBCALL void fb_StrLock(void)
{
    CHECK(lock_depth == 0);
    lock_depth++;
    locks++;
}

FBCALL void fb_StrUnlock(void)
{
    CHECK(lock_depth == 1);
    lock_depth--;
    unlocks++;
}
#endif

FBCALL int fb_ErrorSetNum(int error)
{
    last_error = error;
    return error;
}

FBSTRING_ACCESS *fb_hUStrAlloc_NoLock(ssize_t length)
{
    allocations++;
    requested = length;
#ifdef ENABLE_MT
    CHECK(lock_depth == 1);
#endif
    if (fail_allocation || length < 0 || length > 32) {
        last_error = FB_RTERROR_OUTOFMEM;
        return NULL;
    }
    memset(storage, 0xA5, sizeof(storage));
    descriptor.data = (char *)&storage[1];
    descriptor.len = length | FB_TEMPSTRBIT;
    descriptor.size = length;
    descriptor.data[0] = 0;
    return &descriptor;
}

static void conversion(const FB_WCHAR *input, size_t count, const unsigned char *expected, size_t length)
{
    cases++;
    unsigned long long before = allocations;
    FBSTRING_ACCESS *result = fb_hGfxWideGlyphs(input, count);
    CHECK(result == &descriptor && allocations == before + 1 && requested == (ssize_t)count);
    CHECK(FB_STRSIZE(result) == (ssize_t)length && FB_ISTEMP(result));
    CHECK(memcmp(result->data, expected, length) == 0 && result->data[length] == 0);
    CHECK(storage[0] == 0xA5 && storage[count + 2] == 0xA5 && lock_depth == 0);
}

int main(void)
{
    FB_WCHAR input[2];
    unsigned char expected[2];
    for (unsigned int scalar = 0; scalar <= 0x10FFFF; scalar++) {
        size_t count = 1;
        if (scalar <= 0xFFFF) {
            input[0] = (FB_WCHAR)scalar;
        } else {
            input[0] = (FB_WCHAR)(0xD800u + ((scalar - 0x10000u) >> 10));
            input[1] = (FB_WCHAR)(0xDC00u + ((scalar - 0x10000u) & 0x3FFu));
            count = 2;
        }
        expected[0] = scalar <= 0xFF ? (unsigned char)scalar : '?';
        conversion(input, count, expected, 1);
    }
    static const unsigned int following[] = {0, 1, 0xD7FF, 0xDBFF, 0xE000};
    for (unsigned int high = 0xD800; high <= 0xDBFF; high++)
        for (size_t next = 0; next < sizeof(following) / sizeof(following[0]); next++) {
            input[0] = (FB_WCHAR)high;
            input[1] = (FB_WCHAR)following[next];
            expected[0] = '?';
            expected[1] = following[next] <= 0xFF ? (unsigned char)following[next] : '?';
            conversion(input, 2, expected, 2);
        }
    conversion(NULL, 0, expected, 0);
    unsigned long long before = allocations;
    last_error = 123;
    CHECK(fb_hGfxWideGlyphs(NULL, 1) == NULL && allocations == before && last_error == 123);
    CHECK((size_t)FB_USTRING_MAX_BYTES < SIZE_MAX);
    CHECK(fb_hGfxWideGlyphs(input, (size_t)FB_USTRING_MAX_BYTES + 1) == NULL);
    CHECK(allocations == before && last_error == FB_RTERROR_OUTOFMEM);
    fail_allocation = 1;
    CHECK(fb_hGfxWideGlyphs(input, 1) == NULL && allocations == before + 1);
    CHECK(fb_hGfxWideGlyphs(input, (size_t)FB_USTRING_MAX_BYTES) == NULL);
    CHECK(requested == (ssize_t)FB_USTRING_MAX_BYTES && last_error == FB_RTERROR_OUTOFMEM);
    CHECK(lock_depth == 0 && locks == unlocks);
    printf("GLYPH_UTF16_PASS cases=%llu checks=%llu allocations=%llu\n", cases, checks, allocations);
    return 0;
}

/* end of wide-glyphs.c */
