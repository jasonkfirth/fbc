/*
 * Project: FreeBASIC runtime qualification
 * File: string-header-runtime.c
 * Purpose: Check the production headers together with a rebuilt native runtime.
 * Responsibilities: Verify public wide allocation limits, ordinary string values
 *     and packed descriptors without requesting large valid allocations.
 * This file intentionally does NOT replace the character-width model matrix.
 */
#include "fb.h"

#define CHECK(condition) do { if (!(condition)) { \
    printf("native string header failure at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    const FB_WCHAR source[] = {FB_WCHAR_LITERAL('a'), FB_WCHAR_LITERAL('b'), FB_WCHAR_LITERAL('c'), FB_WCHAR_LITERAL('\0')};
    FB_WCHAR *buffer;

    CHECK(fb_WstrAlloc(-1) == NULL);
    CHECK(fb_WstrAlloc(0) == NULL);
    /* The public function must reach the repaired helper. Its overflow guard
     * rejects this count before it can call the real allocator. */
    CHECK(fb_WstrAlloc(PTRDIFF_MAX) == NULL);
    buffer = fb_WstrAlloc(7);
    CHECK(buffer != NULL);
    fb_wstr_Copy(buffer, source, 3);
    CHECK(fb_wstr_Len(buffer) == 3);
    CHECK(fb_wstr_Compare(buffer, source, 4) == 0);
    fb_wstr_Del(buffer);

    union {
        FBSTRING alignment;
        unsigned char bytes[sizeof(FBSTRING) + 1];
    } packed;
    FBSTRING initial = {NULL, FB_TEMPSTRBIT | 3, 17};
    memcpy(packed.bytes + 1, &initial, sizeof(initial));
    FBSTRING_ACCESS *descriptor = (FBSTRING_ACCESS *)(void *)(packed.bytes + 1);
    fb_hStrSetLength(descriptor, 9);
    CHECK(FB_STRSIZE(descriptor) == 9);
    CHECK(FB_ISTEMP(descriptor));
    CHECK(descriptor->data == NULL && descriptor->size == 17);
    puts("STRING_HEADER_RUNTIME_PASS");
    return 0;
}

/* end of string-header-runtime.c */
