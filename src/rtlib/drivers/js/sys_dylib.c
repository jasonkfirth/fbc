/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_dylib.c
 * Purpose: Dynamic library loading functions.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL void *fb_DylibLoad( FBSTRING *library )
{
	return NULL;
}

FBCALL void *fb_DylibSymbol( void *library, FBSTRING *symbol )
{
	return NULL;
}

FBCALL void *fb_DylibSymbolByOrd( void *library, short int symbol )
{
	return NULL;
}

FBCALL void fb_DylibFree( void *library )
{
}

/* end of drivers/js/sys_dylib.c */
