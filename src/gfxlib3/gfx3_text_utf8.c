/*
    FreeBASIC gfxlib3
    File: gfx3_text_utf8.c
    Purpose: Expose the same Unicode bitmap adapters as gfxlib2.
    Responsibilities: Bind scalar decoding and pattern conversion to gfxlib3.
    This file intentionally does NOT implement rendering or own graphics state.
*/

/* Both renderers expose the same bitmap and packed-pattern C ABI. Compile
   the small common adapters against this library's implementations so their
   Unicode semantics and temporary ownership cannot drift apart. */
#include "../gfxlib2/gfx_drawstring_utf8.c"
#include "../gfxlib2/gfx_pattern_utf8.c"

/* end of gfx3_text_utf8.c */
