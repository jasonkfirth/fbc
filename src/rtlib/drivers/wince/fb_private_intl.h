/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/fb_private_intl.h
 * Purpose: Provide fb private intl runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include <windows.h>

char *fb_hGetLocaleInfo( LCID Locale, LCTYPE LCType, char *pszBuffer, size_t uiSize );
FBSTRING *fb_hIntlConvertString( FBSTRING *source, int source_cp, int dest_cp );

/* end of drivers/wince/fb_private_intl.h */
