/*
 * Project: FreeBASIC runtime qualification
 * File: runtime-type-layout.c
 * Purpose: Record ABI facts independently of internal C structure tag spellings.
 * Responsibilities: Keep public typedef names, record layouts, packed views and
 *     selected offsets visible in a native object for exact before/after checks.
 * This file intentionally does NOT name private tags or depend on runtime values.
 */
#include "fb.h"
#include "fb_private_thread.h"
#include "fb_profile.h"
#include "io_serial_private.h"
#include <stddef.h>

#define LAYOUT(type) sizeof(type), _Alignof(type)
#define OFFSET(type, member) offsetof(type, member)

extern const size_t fb_runtime_type_layout[];
const size_t fb_runtime_type_layout[] = {
    LAYOUT(FBSTRING), LAYOUT(FBSTRING_ACCESS),
    LAYOUT(FBARRAY), LAYOUT(FBARRAY_ACCESS),
    LAYOUT(FBARRAYDIM), LAYOUT(FBARRAYDIM_ACCESS), LAYOUT(FBARRAY_FLAGS),
    LAYOUT(FB_LISTELEM), LAYOUT(FB_LIST),
    LAYOUT(FB_RTERROR), LAYOUT(FB_ERRORCTX),
    LAYOUT(FB_DATADESC), LAYOUT(EVENT),
    LAYOUT(FB_FILE), LAYOUT(FB_FILE_HOOKS), LAYOUT(FB_FILE_ENCOD),
    LAYOUT(FB_RNDSTATE), LAYOUT(FB_RND_ALGORITHMS),
    LAYOUT(FB_RTTI), LAYOUT(FB_BASEVT), LAYOUT(FB_OBJECT),
    LAYOUT(fb_Rect), LAYOUT(fb_Coord), LAYOUT(fb_ConHooks),
    LAYOUT(FB_SERIAL_PARITY), LAYOUT(FB_SERIAL_STOP_BITS),
    LAYOUT(FB_SERIAL_OPTIONS), LAYOUT(FB_COM_STATUS),
    LAYOUT(FBTHREAD), LAYOUT(FBMUTEX), LAYOUT(FBCOND *),
    LAYOUT(FBTHREADFLAGS), LAYOUT(FB_FBTHREADCTX), LAYOUT(FB_TLS_CTX_HEADER),
    LAYOUT(STRING_INFO), LAYOUT(STRING_INFO_TB), LAYOUT(STRING_TABLE),
    LAYOUT(STRING_HASH_TB), LAYOUT(STRING_HASH_TABLE), LAYOUT(STRING_HASH),
    LAYOUT(FB_PROFILER_GLOBAL), LAYOUT(FB_PROFILER_METRICS),
    LAYOUT(W32_SERIAL_INFO),
    OFFSET(FBSTRING, data), OFFSET(FBSTRING, len), OFFSET(FBSTRING, size),
    OFFSET(FBARRAY, data), OFFSET(FBARRAY, ptr), OFFSET(FBARRAY, dimTB),
    OFFSET(FBARRAYDIM, elements), OFFSET(FBARRAYDIM, lbound), OFFSET(FBARRAYDIM, ubound),
    OFFSET(FB_DATADESC, len), OFFSET(EVENT, type),
    OFFSET(FB_RTTI, stdlistVT), OFFSET(FB_RTTI, id), OFFSET(FB_RTTI, pRTTIBase)
};

/* end of runtime-type-layout.c */
