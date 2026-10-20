/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL size_t fb_GetMemAvail( int mode )
{
	return 16 * 1024 * 1024;
}

/* end of drivers/js/sys_fmem.c */
