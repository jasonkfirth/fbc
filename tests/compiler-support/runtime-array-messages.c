/*
    Project: FreeBASIC runtime regression tests
    File: runtime-array-messages.c
    Purpose: Compare array diagnostic truncation with the native message contract.
    Responsibilities: Check complete messages, fixed-buffer limits and format failures.
    This file intentionally does NOT change runtime handlers or use large allocations.
*/
#include "fb.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char observed[FB_ERRMSG_SIZE];
static int observed_error;
static int valid_dispatch = 1;
static int append_calls;
static int fail_call;

int test_snprintf(char *buffer, size_t size, const char *format, ...);

int test_snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args;
    int result;
    ++append_calls;
    if (append_calls == fail_call) {
        if (size != 0)
            buffer[0] = '?';
        return -1;
    }
    va_start(args, format);
    result = vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

FB_ERRHANDLER fb_ErrorThrowMsg(int errnum, int linenum, const char *filename,
    const char *message, void *resume, void *resume_next)
{
    const char *end = memchr(message, '\0', FB_ERRMSG_SIZE);
    observed_error = errnum;
    if (end == NULL || linenum != 42 || filename == NULL ||
        strcmp(filename, "query.bas") != 0 || resume != NULL || resume_next != NULL) {
        valid_dispatch = 0;
        return NULL;
    }
    memcpy(observed, message, (size_t)(end - message) + 1);
    return NULL;
}

static int check_message(int kind, const char *name)
{
    unsigned char storage = 0;
    FBARRAY array = {0};
    char expected[FB_ERRMSG_SIZE];
    int error;
    array.ptr = &storage;
    array.dimensions = 1;
    if (kind == 0) {
        error = FB_RTERROR_OUTOFBOUNDS;
        if (name)
            (void)snprintf(expected, sizeof(expected), "\n'%s' accessed with invalid index = 5, must be between 0 and 3", name);
        else
            (void)snprintf(expected, sizeof(expected), "\narray accessed with invalid index = 5, must be between 0 and 3");
        (void)fb_ArrayBoundChkEx(5, 0, 3, 42, "query.bas", name);
    } else if (kind == 1) {
        error = FB_RTERROR_NOTDIMENSIONED;
        if (name)
            (void)snprintf(expected, sizeof(expected), "\n'%s' not dimensioned and array elements are not allocated", name);
        else
            (void)snprintf(expected, sizeof(expected), "\narray not dimensioned and array elements are not allocated");
        (void)fb_ArrayDimensionChk(1, NULL, 42, "query.bas", name);
    } else {
        error = FB_RTERROR_WRONGDIMENSIONS;
        if (name)
            (void)snprintf(expected, sizeof(expected), "\n'%s' accessed with wrong number of dimensions, 2 given but expected 1", name);
        else
            (void)snprintf(expected, sizeof(expected), "\narray accessed with wrong number of dimensions, 2 given but expected 1");
        (void)fb_ArrayDimensionChk(2, &array, 42, "query.bas", name);
    }
    return valid_dispatch && observed_error == error && strcmp(observed, expected) == 0;
}

int main(void)
{
    static const size_t lengths[] = {0, 3, 1020, 1021, 1022, 1023, 1024, 2048, 4096};
    char name[4097];
    size_t i;
    int kind;
    unsigned int cases = 0;
    for (kind = 0; kind < 3; ++kind) {
        if (!check_message(kind, NULL))
            return 10 + kind;
        ++cases;
        for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
            memset(name, 'n', lengths[i]);
            name[lengths[i]] = '\0';
            if (!check_message(kind, name))
                return 20 + kind;
            ++cases;
        }
    }
#ifdef BASIC_FORMATTER
    /* A deterministic CRT refusal verifies the negative return contract.
       It writes only the first byte of the supplied valid output span. */
    for (fail_call = 1; fail_call <= 3; ++fail_call) {
        const char *expected = fail_call == 1 ? "'values' accessed with invalid index = 5, must be between 0 and 3" :
            fail_call == 2 ? "\naccessed with invalid index = 5, must be between 0 and 3" : "\n'values' ";
        append_calls = 0;
        (void)fb_ArrayBoundChkEx(5, 0, 3, 42, "query.bas", "values");
        if (!valid_dispatch || observed_error != FB_RTERROR_OUTOFBOUNDS || strcmp(observed, expected) != 0)
            return 31 + fail_call;
        ++cases;
    }
#endif
    if (cases != 30
#ifdef BASIC_FORMATTER
        + 3
#endif
        )
        return 40;
    puts("ARRAY_MESSAGES=PASS");
    return 0;
}

/* end of runtime-array-messages.c */
