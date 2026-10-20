/*
 * Project: FreeBASIC runtime library
 * File: fb_legacy.h
 * Purpose: Provide fb legacy runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
FBCALL FBSTRING    *fb_LCASE            ( FBSTRING *src );
FBCALL FBSTRING    *fb_UCASE            ( FBSTRING *src );

FBCALL FB_WCHAR    *fb_WstrLcase        ( const FB_WCHAR *src );
FBCALL FB_WCHAR    *fb_WstrUcase        ( const FB_WCHAR *src );

/* end of fb_legacy.h */
