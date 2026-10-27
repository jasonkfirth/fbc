/* Project: FreeBASIC array-copy tests
 * File: array-copy-hooks.h
 * Purpose: Isolate allocator faults to the resize-to translation unit.
 * Responsibilities: Declare helpers before substituting malloc and calloc.
 * This file intentionally does NOT replace allocation in the runtime archive.
 */
#ifndef FB_TEST_ARRAY_COPY_HOOKS_H
#define FB_TEST_ARRAY_COPY_HOOKS_H
#include "fb.h"
void *fb_CopyFailMalloc(size_t size);
void *fb_CopyFailCalloc(size_t count, size_t size);
int fb_CopyAllocationFailures(void);
#ifdef FB_COPY_ALLOC_INSTRUMENT
#define malloc fb_CopyFailMalloc
#define calloc fb_CopyFailCalloc
#endif
#endif
/* end of array-copy-hooks.h */
