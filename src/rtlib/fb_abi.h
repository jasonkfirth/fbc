/*
 * Project: FreeBASIC runtime and internal C libraries
 * File: fb_abi.h
 * Purpose: Bind ordinary C identifiers to existing FreeBASIC ABI symbols.
 * Responsibilities: Preserve the target C label prefix for data and Cdecl
 *     entry points whose ABI names are reserved or otherwise unusual.
 * This file intentionally does NOT contain: layouts or non-Cdecl decoration.
 */
#ifndef FB_RTLIB_ABI_H
#define FB_RTLIB_ABI_H

/* A C name need not repeat the ABI spelling. The compiler supplies the native
 * prefix, including the extra underscore on Win32 and other prefixed targets. */
#ifndef __USER_LABEL_PREFIX__
#error Runtime ABI labels require a GNU-compatible target label prefix
#endif
#define FB_RTLIB_STRINGIFY_I(value) #value
#define FB_RTLIB_STRINGIFY(value) FB_RTLIB_STRINGIFY_I(value)
#define FB_C_SYMBOL(name) __asm__(FB_RTLIB_STRINGIFY(__USER_LABEL_PREFIX__) name)
#define FB_RTLIB_SYMBOL(name) FB_C_SYMBOL(name)

#endif
/* end of fb_abi.h */
