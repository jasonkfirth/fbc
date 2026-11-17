/*
 * Project: FreeBASIC runtime library
 * File: drivers/linux/sys_getexepath.c
 * Purpose: get the executable path.
 * Responsibilities: Target-specific runtime support for linux.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/stat.h>

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	char *p;
	struct stat finfo;
	ssize_t length;

	if( dst == NULL || maxlen <= 0 )
		return NULL;

	memset( dst, 0, (size_t)maxlen );
	if( maxlen == 1 )
		return NULL;

	/* A full readlink buffer is truncated. A shorter result retains the
	   terminator supplied by the initial zeroing. */
	length = stat("/proc/self/exe", &finfo) == 0 ? readlink("/proc/self/exe", dst, (size_t)maxlen) : -1;
	if( length >= 0 && length < maxlen ) {
		/* Linux-like proc fs is available */
		p = strrchr(dst, '/');
		if (p == dst) /* keep the "/" rather than returning "" */
			*(p + 1) = '\0';
		else if (p)
			*p = '\0';
		else
			dst[0] = '\0';
	} else {
		dst[0] = '\0';
		p = NULL;
	}

	return p;
}

/* end of drivers/linux/sys_getexepath.c */
