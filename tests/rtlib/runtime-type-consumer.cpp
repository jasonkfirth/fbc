/*
 * Project: FreeBASIC runtime qualification
 * File: runtime-type-consumer.cpp
 * Purpose: Exercise C runtime typedefs from a C++ consumer.
 * Responsibilities: Preserve C linkage, descriptor layout and ordinary wide
 *     allocation/copy values while internal C tag spellings change.
 * This file intentionally does NOT export a C++ ABI made from private tags.
 */
extern "C" {
#include "fb.h"
}
#include <stddef.h>

static_assert(sizeof(FBSTRING) == 3 * sizeof(void *), "string descriptor ABI");
static_assert(alignof(FBSTRING_ACCESS) == 1, "packed descriptor access ABI");
static_assert(offsetof(FBARRAY, data) == 0, "array origin ABI");

int main()
{
    char text[] = "tag";
    FBSTRING value = {text, 0, 3};
    fb_hStrSetLength(&value, 3);
    if (FB_STRSIZE(&value) != 3 || value.data != text)
        return 1;
    FB_WCHAR *wide = fb_WstrAlloc(4);
    if (wide == NULL)
        return 2;
    fb_wstr_Copy(wide, FB_WCHAR_LITERAL("tag"), 3);
    const bool equal = fb_wstr_Len(wide) == 3;
    fb_wstr_Del(wide);
    if (!equal)
        return 3;
    puts("CPP_RUNTIME_TYPEDEF_PASS");
    return 0;
}

/* end of runtime-type-consumer.cpp */
