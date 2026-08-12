/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: sys_getexename.c

    Purpose:

        Return the leaf name of the running FreeBASIC program.

    Responsibilities:

        - read the program name saved by fb_Init()
        - remove any UnixLib path prefix
        - copy a null-terminated name into caller-owned storage

    This file intentionally does NOT contain:

        - native RISC OS pathname parsing
        - executable discovery through the filesystem
        - allocation of the destination buffer

    UnixLib path mapping:

        UnixLib supplies argv[0] using its Unix-style path mapping.  The slash
        separator is therefore deliberate even when the underlying RISC OS
        pathname uses native syntax.
*/

#include "../fb.h"

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	const char *name;
	const char *slash;

	if( (dst == NULL) || (maxlen <= 1) )
		return NULL;

	dst[0] = '\0';

	if( (__fb_ctx.argc <= 0) || (__fb_ctx.argv == NULL) ||
	    (__fb_ctx.argv[0] == NULL) )
		return NULL;

	name = __fb_ctx.argv[0];
	slash = strrchr( name, '/' );
	if( slash != NULL )
		name = slash + 1;

	strncpy( dst, name, (size_t)maxlen - 1 );
	dst[maxlen - 1] = '\0';

	return dst;
}

/* end of sys_getexename.c */
