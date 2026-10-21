/*
    FreeBASIC classic AmigaOS allocator qualification
    ------------------------------------------------
    File: allocator-hooks.h
    Purpose: Route a private test allocator's Exec requests through a guard.
    Responsibilities: Preserve the native ABI while allowing bounded failures.
    This file intentionally does NOT replace the production allocation API.
*/

#ifndef FB_AMIGA_ALLOCATOR_TEST_HOOKS_H
#define FB_AMIGA_ALLOCATOR_TEST_HOOKS_H

#include "../../src/rtlib/fb.h"
#include <proto/exec.h>

APTR amiga_test_allocate(ULONG bytes, ULONG requirements);

/* Preinclude this only when compiling the probe's allocator object. The
   bridge retains the real Exec inline and forwards ordinary requests. */
#undef AllocMem
#define AllocMem(bytes, requirements) amiga_test_allocate((bytes), (requirements))

#endif

/* end of allocator-hooks.h */
