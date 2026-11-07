/*
 * Project: FreeBASIC C backend regression tests
 * File: c-record-pointer-dependencies.c
 * Purpose: Supply ordinary C declarations for opaque and completed records.
 * Responsibilities: Observe pointer storage and exchange an aggregate by value.
 * This file intentionally does NOT include generated record definitions.
 */
#include <stdint.h>
#include <stddef.h>

#if !defined(FB_TEST_OPAQUE) || !defined(FB_TEST_POINTER_HEADER) || !defined(FB_TEST_COMPLETED)
#error The emitted BASIC record tags must be supplied by the test driver
#endif

struct FB_TEST_OPAQUE;
struct FB_TEST_POINTER_HEADER {
    struct FB_TEST_OPAQUE *OPAQUE;
    void (*CALLBACK)(struct FB_TEST_OPAQUE *);
};

struct FB_TEST_COMPLETED {
    uint8_t PREFIX;
    int32_t VALUE;
};

_Static_assert(offsetof(struct FB_TEST_COMPLETED, VALUE) == 4, "value offset");
_Static_assert(sizeof(struct FB_TEST_COMPLETED) == 8, "value extent");

int32_t fb_test_record_pointer_accept(struct FB_TEST_POINTER_HEADER *value);
void fb_test_record_pointer_prepare(struct FB_TEST_POINTER_HEADER *value);
struct FB_TEST_COMPLETED fb_test_record_pointer_change(struct FB_TEST_COMPLETED value);

int32_t fb_test_record_pointer_accept(struct FB_TEST_POINTER_HEADER *value)
{
    return value != NULL && value->OPAQUE == NULL && value->CALLBACK == NULL;
}

void fb_test_record_pointer_prepare(struct FB_TEST_POINTER_HEADER *value)
{
    value->OPAQUE = NULL;
    value->CALLBACK = NULL;
}

struct FB_TEST_COMPLETED fb_test_record_pointer_change(struct FB_TEST_COMPLETED value)
{
    /* The fixture supplies VALUE 23 or 52, keeping this addition representable. */
    value.PREFIX = (uint8_t)(value.PREFIX + 1u);
    value.VALUE += 29;
    return value;
}

/* end of c-record-pointer-dependencies.c */
