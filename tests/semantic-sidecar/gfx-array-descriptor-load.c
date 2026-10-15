/*
 * Project: FreeBASIC compiler graphics argument tests
 * File: gfx-array-descriptor-load.c
 * Purpose: Observe GET's array-address lowering without creating a display.
 * Responsibilities: Check the native destination and array descriptor ABI.
 * This file does not render pixels or test the graphics runtime.
 * The test runs on one thread; its captured state is process-local.
 */

#include "../../src/rtlib/fb.h"

/* The real entry point receives this descriptor layout from compiled BASIC. */
static void *last_destination;
static int64_t last_lower_bound;
static unsigned int calls;
static int valid_descriptor;

FBCALL int fb_GfxGet(void *target, float x1, float y1, float x2, float y2,
                    unsigned char *destination, int coord_type, FBARRAY *array)
{
    (void)target;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)coord_type;

    calls++;
    last_destination = destination;
    valid_descriptor = array != NULL && array->dimensions == 1;
    if (valid_descriptor) {
        valid_descriptor = destination == array->ptr;
        last_lower_bound = array->dimTB[0].lbound;
    }
    return 0;
}

void *compiler_test_gfx_destination(void)
{
    return last_destination;
}

int64_t compiler_test_gfx_lower_bound(void)
{
    return last_lower_bound;
}

unsigned int compiler_test_gfx_calls(void)
{
    return calls;
}

int compiler_test_gfx_valid(void)
{
    return valid_descriptor;
}

/* end of gfx-array-descriptor-load.c */
