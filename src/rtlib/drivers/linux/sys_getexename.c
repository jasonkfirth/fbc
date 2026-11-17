/*
 * Project: FreeBASIC runtime library
 * File: drivers/linux/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for linux.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/stat.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	char *p;
	char linkname[1024];
	struct stat finfo;
	ssize_t length;

	if( dst == NULL || maxlen <= 0 )
		return NULL;

	memset( dst, 0, (size_t)maxlen );
	if( maxlen == 1 )
		return NULL;

	sprintf(linkname, "/proc/%d/exe", getpid());
	/* readlink omits the terminator. Using the full capacity distinguishes
	   an exact fit with terminator space from a truncated executable path.
	   The initial zeroing supplies the terminator after a fitting result. */
	length = stat(linkname, &finfo) == 0 ? readlink(linkname, dst, (size_t)maxlen) : -1;
	if( length >= 0 && length < maxlen ) {
		/* Linux-like proc fs is available */
		p = strrchr(dst, '/');
		if (p != NULL)
			++p;
		else
			p = dst;
	} else {
		dst[0] = '\0';
		p = NULL;
	}

	return p;
}

/* end of drivers/linux/sys_getexename.c */
