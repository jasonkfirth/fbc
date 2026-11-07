/*
    Project: FreeBASIC C backend regression tests
    File: c-wide-libc-native.c
    Purpose: Check the original native wide-function address and callback type.
    Responsibilities: Return the C library entry through its exact function type.
    This file intentionally does NOT wrap or replace the native function.
*/
#include <stddef.h>
#include <wchar.h>

typedef size_t (*NativeWideLength)(const wchar_t *);
typedef wchar_t *(*NativeWideSearch)(const wchar_t *, wchar_t);
NativeWideLength native_wide_length(void);
NativeWideSearch native_wide_search(void);

NativeWideLength native_wide_length(void)
{
    return &wcslen;
}

NativeWideSearch native_wide_search(void)
{
    return &wcschr;
}

/* end of c-wide-libc-native.c */
