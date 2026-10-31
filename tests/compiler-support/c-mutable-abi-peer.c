/*
    Project: FreeBASIC C backend regression controls
    File: c-mutable-abi-peer.c
    Purpose: Observe a mutable pointer passed from a generated assignment.
    Responsibilities: Check address identity, read the original value and write
    through the exact int32_t pointer declared by the BASIC caller.
    This file intentionally does NOT contain: Emitter helpers or casts that
    change the argument's qualifiers.
*/
#include <stdint.h>
#include <stdlib.h>

int32_t fb_c_mutable_touch(int32_t *value, uintptr_t expected);

int32_t fb_c_mutable_touch(int32_t *value, uintptr_t expected)
{
    int32_t result;

    if (value == NULL || (uintptr_t)value != expected)
        abort();

    result = *value;
    if (result != 41 && result != 42)
        abort();

    *value = result + 1;
    return result;
}

/* end of c-mutable-abi-peer.c */
