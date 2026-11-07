/*
 * Project: FreeBASIC runtime ABI qualification
 * File: runtime-symbol-current.c
 * Purpose: Check new C spellings against legacy objects and runtime binaries.
 * Responsibilities: Compare live addresses and constructor values without
 *     changing context state or performing allocation-heavy runtime operations.
 * This file intentionally does NOT depend on internal C++ name mangling.
 */
#include "fb.h"

#ifdef __cplusplus
extern "C" {
#endif
const void *fb_symbol_legacy_address(int slot);
void fb_symbol_legacy_construct(FB_OBJECT *value);
void fb_symbol_legacy_copy(FB_OBJECT *value, const FB_OBJECT *source);
#ifdef __cplusplus
}
#endif

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

int main(void)
{
    CHECK(fb_symbol_legacy_address(0) == &fb_ctx);
    CHECK(fb_symbol_legacy_address(1) == fb_errmsg);
    CHECK(fb_symbol_legacy_address(2) == &fb_data_ptr);
    CHECK(fb_symbol_legacy_address(3) == fb_utf8_bmarkTb);
    CHECK(fb_utf8_trailingTb[0] == 0);
    CHECK(fb_utf8_offsetsTb[0] == 0);
    CHECK(fb_ObjectRTTI.id != NULL);
    FB_OBJECT first, second, third, fourth;
    fb_symbol_legacy_construct(&first);
    fb_ObjectCtor(&second);
    CHECK(first.pVT != NULL && first.pVT == second.pVT);
    fb_symbol_legacy_copy(&third, &first);
    fb_ObjectCopyCtor(&fourth, &second);
    CHECK(third.pVT == fourth.pVT && third.pVT == first.pVT);
    puts("RUNTIME_SYMBOL_MIXED_ABI_PASS");
    return 0;
}

/* end of runtime-symbol-current.c */
