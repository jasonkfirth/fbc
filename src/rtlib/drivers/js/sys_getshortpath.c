/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_getshortpath.c
 * Purpose: Provide sys getshortpath runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetShortPath( char *src, char *dst, ssize_t maxlen )
{
	strncpy(dst, src, maxlen);
	return dst;
}

/* end of drivers/js/sys_getshortpath.c */
