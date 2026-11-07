/*
 * Project: FreeBASIC runtime library
 * File: fb_legacy.h
 * Purpose: Provide fb legacy runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
FBCALL FBSTRING_ACCESS    *fb_LCASE            ( FBSTRING_ACCESS *src );
FBCALL FBSTRING_ACCESS    *fb_UCASE            ( FBSTRING_ACCESS *src );

FBCALL FB_WCHAR    *fb_WstrLcase        ( const FB_WCHAR *src );
FBCALL FB_WCHAR    *fb_WstrUcase        ( const FB_WCHAR *src );

/* end of fb_legacy.h */
