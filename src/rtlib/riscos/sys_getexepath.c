/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: sys_getexepath.c

    Purpose:

        Return the directory portion of the running program name.

    Responsibilities:

        - read argv[0] saved by fb_Init()
        - preserve the root directory when argv[0] starts with a slash
        - reject paths that do not fit in caller-owned storage

    This file intentionally does NOT contain:

        - native RISC OS pathname parsing
        - canonicalization of symbolic links
        - allocation of the destination buffer

    UnixLib path mapping:

        argv[0] uses UnixLib's slash-separated path mapping.  If it contains no
        directory, the current working directory is the best information the
        process has because RISC OS does not provide a /proc executable link.
*/

#include "../fb.h"

#include <unistd.h>

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	const char *slash;
	size_t length;

	if( (dst == NULL) || (maxlen <= 1) )
		return NULL;

	dst[0] = '\0';

	if( (__fb_ctx.argc <= 0) || (__fb_ctx.argv == NULL) ||
	    (__fb_ctx.argv[0] == NULL) )
		return NULL;

	slash = strrchr( __fb_ctx.argv[0], '/' );
	if( slash == NULL )
		return getcwd( dst, (size_t)maxlen );

	length = (size_t)(slash - __fb_ctx.argv[0]);
	if( length == 0 )
		length = 1;

	if( length >= (size_t)maxlen )
		return NULL;

	memcpy( dst, __fb_ctx.argv[0], length );
	dst[length] = '\0';

	return dst;
}

/* end of sys_getexepath.c */
