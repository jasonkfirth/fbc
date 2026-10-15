/*
    FreeBASIC native structure-return regression
    File: struct-return-store.c
    Purpose: Return genuine C ABI aggregates to pointer and array destinations.
    Responsibilities: Floating, integer and mixed register-return classes.
    This file contains no compiler emitter or replacement ABI.
*/
#include <stdint.h>
struct Float3 { float x,y,z; };
struct Integer2 { uint64_t x,y; };
struct MixedRX { uint64_t x; double y; };
struct MixedXR { double x; uint64_t y; };
struct Float3 make_float3(void) { struct Float3 v={1.25f,-2.5f,3.75f};return v; }
struct Integer2 make_integer2(void) { struct Integer2 v={UINT64_C(0x11223344aabbccdd),UINT64_C(0x8877665544332211)};return v; }
struct MixedRX make_mixed_rx(void) { struct MixedRX v={UINT64_C(0x11223344aabbccdd),-2.5};return v; }
struct MixedXR make_mixed_xr(void) { struct MixedXR v={1.25,UINT64_C(0x8877665544332211)};return v; }
/* end of struct-return-store.c */
