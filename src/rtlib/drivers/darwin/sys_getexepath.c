/*
 * Project: FreeBASIC runtime library
 * File: drivers/darwin/sys_getexepath.c
 * Purpose: get the executable path.
 * Responsibilities: Target-specific runtime support for darwin.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "mach-o/dyld.h"

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	char *p;
	uint32_t len = maxlen;

	if (_NSGetExecutablePath(dst, &len) == 0) {
		dst[len] = '\0';
		p = strrchr(dst, '/');
		if (p == dst) /* keep the "/" rather than returning "" */
			*(p + 1) = '\0';
		else if (p) {
			*p = '\0';
			/* OS X likes to append "/." to the path, so remove it */
			if (*(p-2) == '/' && *(p-1) == '.')
				*(p-2) = '\0';
		} else
			dst[0] = '\0';
	} else {
		p = NULL;
	}

	return p;
}

/* end of drivers/darwin/sys_getexepath.c */
