/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/array-allocation-hooks.h
 * Purpose: Isolate allocator fault injection to the array allocator object.
 * Responsibilities: Read normal runtime/libc declarations, declare the test
 *     hooks, then optionally substitute calls in the allocator translation unit.
 * This file intentionally does NOT replace allocators in the runtime archive.
 */
#ifndef FB_TEST_ARRAY_ALLOCATION_H
#define FB_TEST_ARRAY_ALLOCATION_H

#include "fb.h"

void *fb_ArrayAllocTestMalloc( size_t size );
void *fb_ArrayAllocTestCalloc( size_t count, size_t size );
void *fb_ArrayAllocTestMemset( void *ptr, int value, size_t size );

#ifdef FB_ARRAY_ALLOC_INSTRUMENT
#define malloc fb_ArrayAllocTestMalloc
#define calloc fb_ArrayAllocTestCalloc
#define memset fb_ArrayAllocTestMemset
#endif

#endif
/* end of rtlib/array-allocation-hooks.h */
