/*
 * Project: FreeBASIC runtime ABI qualification
 * File: runtime-symbol-legacy.c
 * Purpose: Compile an unchanged ABI consumer against the preceding headers.
 * Responsibilities: Return legacy symbol addresses and call legacy constructors
 *     from a separate translation unit for mixed-header link checks.
 * This file intentionally does NOT use the new ordinary C identifier spellings.
 */
#include "fb.h"

const void *fb_symbol_legacy_address(int slot);
void fb_symbol_legacy_construct(FB_OBJECT *value);
void fb_symbol_legacy_copy(FB_OBJECT *value, const FB_OBJECT *source);

const void *fb_symbol_legacy_address(int slot)
{
    switch (slot) {
    case 0: return &__fb_ctx;
    case 1: return __fb_errmsg;
    case 2: return &__fb_data_ptr;
    case 3: return __fb_utf8_bmarkTb;
    default: return NULL;
    }
}

void fb_symbol_legacy_construct(FB_OBJECT *value)
{
    _ZN10fb_Object$C1Ev(value);
}

void fb_symbol_legacy_copy(FB_OBJECT *value, const FB_OBJECT *source)
{
    _ZN10fb_Object$C1ERKS_(value, source);
}

/* end of runtime-symbol-legacy.c */
