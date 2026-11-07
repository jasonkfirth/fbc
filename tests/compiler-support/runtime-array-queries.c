/*
    Project: FreeBASIC runtime regression tests
    File: runtime-array-queries.c
    Purpose: Check physical storage identity and 1-based bound queries.
    Responsibilities: Cover zero origins, empty arrays, extreme dimensions and errors.
    This file intentionally does NOT allocate large arrays or install error handlers.
*/
#include "fb.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* This single-threaded fixture observes error dispatch without changing the
   process-wide runtime handler. The original formatting and caller ABI run. */
static int observed_error;
static int dispatch_valid = 1;

FB_ERRHANDLER fb_ErrorThrowMsg(int errnum, int linenum, const char *filename,
    const char *message, void *resume, void *resume_next)
{
    observed_error = errnum;
    if (linenum != 42 || filename == NULL || message == NULL ||
        resume != NULL || resume_next != NULL)
        dispatch_valid = 0;
    return NULL;
}

static int bounds_match(FBARRAY *array, ssize_t dimension, ssize_t lower, ssize_t upper)
{
    return fb_ArrayLBound(array, dimension) == lower &&
        fb_ArrayUBound(array, dimension) == upper;
}

int main(int argc, char **argv)
{
    unsigned char storage[2] = {17, 25};
    FBARRAY array = {0};
    ssize_t maximum = (ssize_t)(SIZE_MAX >> 1);
    ssize_t minimum = -maximum - 1;
    ssize_t lower = -7;

    if (argc != 2)
        return 2;
    array.ptr = storage;
    array.size = sizeof(storage);
    array.element_len = sizeof(storage[0]);
    array.dimensions = 1;
    array.flags = 1;
    array.dimTB[0].elements = sizeof(storage);
    if (strcmp(argv[1], "zero") == 0 || strcmp(argv[1], "zero-dimension") == 0) {
        if ((uintptr_t)storage > (uintptr_t)maximum - 1)
            return 3;
        lower = (ssize_t)(uintptr_t)storage;
    }
    array.dimTB[0].lbound = lower;
    array.dimTB[0].ubound = lower + 1;
    array.data = (void *)((uintptr_t)storage - (uintptr_t)lower);

    if (strcmp(argv[1], "zero") == 0 || strcmp(argv[1], "ordinary") == 0) {
        if (!bounds_match(&array, 0, 1, 1) ||
            !bounds_match(&array, 1, lower, lower + 1) ||
            !bounds_match(&array, -1, 0, -1) || !bounds_match(&array, 2, 0, -1))
            return 11;
        if (fb_ArrayLen(&array) != 2 || fb_ArraySize(&array) != sizeof(storage))
            return 12;
        observed_error = 0;
        (void)fb_ArrayDimensionChk(1, &array, 42, "query.bas", "values");
        if (observed_error != 0)
            return 13;
    } else if (strcmp(argv[1], "zero-dimension") == 0) {
        (void)fb_ArrayDimensionChk(1, &array, 42, "query.bas", "values");
        if (observed_error != 0)
            return 13;
    } else if (strcmp(argv[1], "empty") == 0) {
        array.data = NULL;
        array.ptr = NULL;
        array.size = 0;
        if (!bounds_match(&array, 0, 1, 0) || !bounds_match(&array, 1, 0, -1))
            return 21;
        observed_error = 0;
        (void)fb_ArrayDimensionChk(1, &array, 42, "query.bas", "values");
        if (observed_error != FB_RTERROR_NOTDIMENSIONED)
            return 22;
    } else if (strcmp(argv[1], "extreme") == 0) {
        if (!bounds_match(&array, minimum, 0, -1) || !bounds_match(&array, maximum, 0, -1))
            return 31;
    } else if (strcmp(argv[1], "dimension") == 0) {
        (void)fb_ArrayDimensionChk(2, &array, 42, "query.bas", "values");
        if (observed_error != FB_RTERROR_WRONGDIMENSIONS)
            return 41;
        observed_error = 0;
        (void)fb_ArrayDimensionChk(1, NULL, 42, "query.bas", "values");
        if (observed_error != FB_RTERROR_NOTDIMENSIONED)
            return 42;
    } else if (strcmp(argv[1], "indices") == 0) {
        (void)fb_ArrayBoundChkEx(lower, lower, lower + 1, 42, "query.bas", "values");
        if (observed_error != 0)
            return 51;
        (void)fb_ArrayBoundChkEx(lower - 1, lower, lower + 1, 42, "query.bas", "values");
        if (observed_error != FB_RTERROR_OUTOFBOUNDS)
            return 52;
        observed_error = 0;
        (void)fb_ArraySngBoundChk(0, 1, 42, "query.bas");
        if (observed_error != 0)
            return 53;
        (void)fb_ArraySngBoundChkEx(2, 1, 42, "query.bas", "values");
        if (observed_error != FB_RTERROR_OUTOFBOUNDS)
            return 54;
    } else {
        return 4;
    }
    if (!dispatch_valid || storage[0] != 17 || storage[1] != 25)
        return 90;
    puts("ARRAY_QUERY=PASS");
    return 0;
}

/* end of runtime-array-queries.c */
