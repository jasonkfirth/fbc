/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: fb_gfx3.h

    Purpose:

        Provide the common internal definitions used by gfxlib3 modules.

    Responsibilities:

        - include the FreeBASIC runtime services used by gfxlib3
        - define fixed-width gfxlib3 result and handle types
        - provide checked size-arithmetic helpers

    This file intentionally does NOT contain:

        - the public FreeBASIC graphics API
        - renderer backend declarations
        - platform window-system declarations
*/

#ifndef __FB_GFX3_H__
#define __FB_GFX3_H__

/*
	The established runtime headers intentionally preserve ABI-era enum and
	string arithmetic which predate gfxlib3's stricter warning policy. Keep
	those inherited diagnostics local to the runtime instead of weakening
	conversion checks for gfxlib3 itself.
*/
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
#pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "../rtlib/fb.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t FB_GFX3_HANDLE;

/*
	The compiler's fbgfx.bi exports these numeric values to BASIC programs.
	Keeping the C-side definitions here lets platform adapters publish exactly
	the same GETXPAD button and status words without importing gfxlib2 headers.
*/
#define XPAD_STATUS_MISSING        0
#define XPAD_STATUS_CONNECTED      1
#define XPAD_STATUS_DISCONNECTED   2

#define XPAD_BUTTON_A              0x00000001
#define XPAD_BUTTON_B              0x00000002
#define XPAD_BUTTON_X              0x00000004
#define XPAD_BUTTON_Y              0x00000008
#define XPAD_BUTTON_L1             0x00000010
#define XPAD_BUTTON_R1             0x00000020
#define XPAD_BUTTON_L3             0x00000040
#define XPAD_BUTTON_R3             0x00000080
#define XPAD_BUTTON_START          0x00000100
#define XPAD_BUTTON_SELECT         0x00000200
#define XPAD_BUTTON_GUIDE          0x00000400
#define XPAD_BUTTON_L2             0x00000800
#define XPAD_BUTTON_R2             0x00001000

#define XPAD_DPAD_UP               0x00000001
#define XPAD_DPAD_RIGHT            0x00000002
#define XPAD_DPAD_DOWN             0x00000004
#define XPAD_DPAD_LEFT             0x00000008

enum FB_GFX3_RESULT {
	FB_GFX3_OK = 0,
	FB_GFX3_INVALID = -1,
	FB_GFX3_OUT_OF_MEMORY = -2,
	FB_GFX3_CLOSED = -3,
	FB_GFX3_FAILED = -4,
	FB_GFX3_EXHAUSTED = -5,
	FB_GFX3_UNSUPPORTED = -6
};

static __inline__ int fb_gfx3_size_add(size_t a, size_t b, size_t *result)
{
	if ((result == NULL) || (a > (SIZE_MAX - b)))
		return FB_GFX3_INVALID;

	*result = a + b;
	return FB_GFX3_OK;
}

static __inline__ int fb_gfx3_size_multiply(size_t a, size_t b, size_t *result)
{
	if (result == NULL)
		return FB_GFX3_INVALID;

	if ((a != 0) && (b > (SIZE_MAX / a)))
		return FB_GFX3_INVALID;

	*result = a * b;
	return FB_GFX3_OK;
}

#endif

/* end of fb_gfx3.h */
