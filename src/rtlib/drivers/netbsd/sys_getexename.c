/*
 * Project: FreeBASIC runtime library
 * File: drivers/netbsd/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for netbsd.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/stat.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	char *p;
	struct stat finfo;
	ssize_t len;
	
	if ((stat("/proc/curproc/exe", &finfo) == 0) && ((len = readlink("/proc/curproc/exe", dst, maxlen - 1)) > -1)) {
		/* NetBSD-like proc fs is available */
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

/* end of drivers/netbsd/sys_getexename.c */
