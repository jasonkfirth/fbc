/* Project: FreeBASIC array-copy tests
 * File: array-copy-hooks.c
 * Purpose: Fail allocations requested by the instrumented resize-to unit.
 * Responsibilities: Count faults while ordinary source allocation keeps working.
 * This file intentionally does NOT replace allocation in the runtime archive.
 * The counter belongs to this single-threaded test process.
 */
#include "array-copy-hooks.h"
static int allocation_failures;
void *fb_CopyFailMalloc(size_t size)
{
    (void)size;
    ++allocation_failures;
    return NULL;
}
void *fb_CopyFailCalloc(size_t count, size_t size)
{
    (void)count;
    (void)size;
    ++allocation_failures;
    return NULL;
}
int fb_CopyAllocationFailures(void)
{
    return allocation_failures;
}
/* end of array-copy-hooks.c */
