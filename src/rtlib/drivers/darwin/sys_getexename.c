/*
 * Project: FreeBASIC runtime library
 * File: drivers/darwin/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for darwin.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "mach-o/dyld.h"

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	char *p;
	uint32_t len = maxlen;

	if (_NSGetExecutablePath(dst, &len) == 0) {
		dst[len] = '\0';
		p = strrchr(dst, '/');
		if (p != NULL)
			++p;
		else
			p = dst;
	} else {
		p = NULL;
	}

	return p;
}

/* end of drivers/darwin/sys_getexename.c */
