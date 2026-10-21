/*
    FreeBASIC classic AmigaOS allocator qualification
    ------------------------------------------------
    File: allocator-bridge.c
    Purpose: Exercise the real Exec-backed C heap and incremental growth.
    Responsibilities: Check contents, bounded movement, shrinking and failures.
    This file intentionally does NOT contain a replacement heap or DOS hooks.
*/

#include "../../src/rtlib/fb.h"
#include <errno.h>
#include <exec/memory.h>
#include <reent.h>
#include <proto/exec.h>

static struct Task *limited_task;
static ULONG allocation_limit, last_request;
static unsigned int rejections;

/* Only this probe's task is limited, and only during a selected operation.
   The private allocator object uses this wrapper instead of the Exec inline.
   Ordinary requests, including SDK startup, reach the real allocator. */
APTR amiga_test_allocate(ULONG bytes, ULONG requirements)
{
    int reject;
    Forbid();
    last_request = bytes;
    reject = limited_task != NULL && FindTask(NULL) == limited_task && bytes > allocation_limit;
    if (reject) ++rejections;
    Permit();
    return reject ? NULL : AllocMem(bytes, requirements);
}

static void limit_allocations(ULONG bytes)
{
    Forbid();
    allocation_limit = bytes;
    rejections = 0;
    limited_task = FindTask(NULL);
    Permit();
}

static unsigned int allow_allocations(void)
{
    unsigned int count;
    Forbid();
    limited_task = NULL;
    count = rejections;
    Permit();
    return count;
}

static unsigned char pattern(size_t offset)
{
    return (unsigned char)((offset * 37u) ^ (offset >> 3));
}

int amiga_test_allocator(void)
{
    unsigned char *memory, *replacement;
    size_t size, offset, previous_size;
    uintptr_t previous_address;
    unsigned int moved = 0;
    volatile size_t impossible = SIZE_MAX;
    size_t overhead;
    int result = 0;

    memory = _malloc_r(_REENT, 64);
    if (memory == NULL) return 39;
    memset(memory, 0x5a, 64);
    if (last_request < 64 || last_request > 1024) { result = 60; goto done; }
    overhead = last_request - 64;
    /* A payload plus header can fit in ULONG while Exec's block rounding
       would overflow. These requests must fail before reaching AllocMem. */
    for (offset = 0; offset < MEM_BLOCKMASK; ++offset) {
        impossible = SIZE_MAX - overhead - offset;
        errno = 0;
        limit_allocations(0);
        replacement = _malloc_r(_REENT, impossible);
        moved = allow_allocations();
        if (replacement != NULL) { free(replacement); result = 67; goto done; }
        if (moved != 0 || errno != ENOMEM) { result = 68; goto done; }
    }
    impossible = SIZE_MAX;
    previous_address = (uintptr_t)memory;
    replacement = realloc(memory, 64);
    if (replacement == NULL) { free(memory); return 40; }
    memory = replacement;
    /* This is the native allocator's capacity policy, not a general C API
       promise. Save addresses as integers before realloc ends an object. */
    if ((uintptr_t)memory != previous_address) { result = 40; goto done; }
    for (offset = 0; offset < 64; ++offset)
        if (memory[offset] != 0x5a) { result = 41; goto done; }

    errno = 0;
    replacement = realloc(memory, impossible);
    if (replacement != NULL) { memory = replacement; result = 42; goto done; }
    if (errno != ENOMEM) { result = 43; goto done; }
    for (offset = 0; offset < 64; ++offset)
        if (memory[offset] != 0x5a) { result = 44; goto done; }
    free(memory);
    memory = calloc(257, 1);
    if (memory == NULL) return 45;
    for (offset = 0; offset < 257; ++offset)
        if (memory[offset] != 0) { result = 46; goto done; }
    free(memory);
    memory = NULL;
    errno = 0;
    replacement = calloc(impossible, 2);
    if (replacement != NULL) { free(replacement); return 47; }
    if (errno != ENOMEM) return 48;

    previous_size = 0;
    for (size = 64; size <= 256000; size += 64) {
        previous_address = (uintptr_t)memory;
        replacement = realloc(memory, size);
        if (replacement == NULL) { result = 49; goto done; }
        memory = replacement;
        if ((uintptr_t)memory != previous_address) ++moved;
        if (moved > 64) { result = 50; goto done; }
        if (previous_size != 0 &&
            (memory[0] != pattern(0) || memory[previous_size - 1] != pattern(previous_size - 1))) {
            result = 51;
            goto done;
        }
        for (offset = previous_size; offset < size; ++offset) memory[offset] = pattern(offset);
        previous_size = size;
    }
    for (offset = 0; offset < previous_size; ++offset)
        if (memory[offset] != pattern(offset)) { result = 52; goto done; }

    replacement = realloc(memory, 64);
    if (replacement == NULL) { result = 53; goto done; }
    memory = replacement;
    for (offset = 0; offset < 64; ++offset)
        if (memory[offset] != pattern(offset)) { result = 54; goto done; }
    replacement = realloc(memory, 0);
    memory = replacement;
    if (replacement != NULL) { result = 55; goto done; }

    /* The reentrant startup interface also permits a missing errno context. */
    memory = _malloc_r(NULL, 32);
    if (memory == NULL) return 56;
    memset(memory, 0xa6, 32);
    replacement = _realloc_r(NULL, memory, 48);
    if (replacement == NULL) { result = 57; goto done; }
    memory = replacement;
    for (offset = 0; offset < 32; ++offset)
        if (memory[offset] != 0xa6) { result = 58; goto done; }
    free(memory);
    memory = _malloc_r(_REENT, 64);
    if (memory == NULL) return 59;
    if (last_request < 64 || last_request > 1024) { result = 60; goto done; }
    overhead = last_request - 64;
    memset(memory, 0x5a, 64);


    /* Reject the optional reservation while allowing the exact allocation.
       A successful fallback must preserve the caller's prior errno. */
    errno = EDOM;
    limit_allocations((ULONG)(65 + overhead));
    replacement = _realloc_r(_REENT, memory, 65);
    moved = allow_allocations();
    if (replacement == NULL) { result = 61; goto done; }
    memory = replacement;
    if (moved != 1 || errno != EDOM) { result = 62; goto done; }
    for (offset = 0; offset < 64; ++offset)
        if (memory[offset] != 0x5a) { result = 63; goto done; }
    memory[64] = 0xa5;

    limit_allocations(0);
    replacement = _realloc_r(_REENT, memory, 66);
    moved = allow_allocations();
    if (replacement != NULL) { memory = replacement; result = 64; goto done; }
    if (moved != 2 || errno != ENOMEM) { result = 65; goto done; }
    for (offset = 0; offset < 65; ++offset)
        if (memory[offset] != (offset == 64 ? 0xa5 : 0x5a)) { result = 66; goto done; }

done:
    free(memory);
    return result;
}

/* end of allocator-bridge.c */
