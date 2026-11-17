/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_decode_vendor.h

    Purpose:

        Include the bundled decoder implementations through a system-header
        boundary so diagnostics remain focused on sfxlib-owned code.

    Responsibilities:

        - include the configured dr_wav and dr_mp3 implementation headers
        - include stb_vorbis as its single-file implementation
        - provide stb_vorbis with the target alloca intrinsic when needed
        - keep upstream decoder diagnostics out of the caller's warning set

    This file intentionally does NOT contain:

        - decoder configuration macros, which are set by sfx_decode.c
        - file loading or decode dispatch
        - changes to bundled upstream source files
*/

#ifndef FB_SFX_DECODE_VENDOR_H
#define FB_SFX_DECODE_VENDOR_H

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC system_header
#endif

#if defined(__GNUC__) && !defined(alloca) && !defined(__MINGW32__) \
	&& !defined(__linux__) && !defined(__linux) && !defined(__sun__) \
	&& !defined(__EMSCRIPTEN__) && !defined(__NEWLIB__)
#define alloca(size) __builtin_alloca(size)
#endif

#include "third_party/dr_wav.h"
#include "third_party/dr_mp3.h"

#if defined(__clang_analyzer__)
#define STB_VORBIS_HEADER_ONLY
#endif
/* NOLINTNEXTLINE(bugprone-suspicious-include): stb_vorbis is a single-file implementation. */
#include "third_party/stb_vorbis.c"
#if defined(__clang_analyzer__)
#undef STB_VORBIS_HEADER_ONLY
#endif

#endif

/* end of sfx_decode_vendor.h */
