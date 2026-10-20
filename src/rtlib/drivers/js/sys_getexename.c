/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	dst[0] = '\0';
	return dst;
}

/* end of drivers/js/sys_getexename.c */
