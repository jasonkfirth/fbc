/*
 * Project: FreeBASIC runtime library
 * File: drivers/solaris/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for solaris.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/stat.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	char *p;
	char linkname[1024];
	struct stat finfo;
	ssize_t len;

	sprintf(linkname, "/proc/%lu/path/a.out", getpid());
	if ((stat(linkname, &finfo) == 0) && ((len = readlink(linkname, dst, maxlen - 1)) > -1)) {
		/* Solaris-like proc fs is available */
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

/* end of drivers/solaris/sys_getexename.c */
