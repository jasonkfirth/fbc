/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_getexepath.c
 * Purpose: get the executable path.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	char *p, *program;
	size_t argv_len, cwd_len, len, capacity;

	if( dst == NULL || maxlen <= 0 || fb_ctx.argv == NULL ||
	    fb_ctx.argv[0] == NULL || __fb_startup_cwd == NULL )
		return NULL;

	program = fb_ctx.argv[0];
	argv_len = strlen( program );
	capacity = (size_t)maxlen;

	/* check for drive letter - if there, get full path from argv[0] */
	if( isalpha((unsigned char)program[0]) && program[1] == ':' )
	{
		len = argv_len;
		if( len >= capacity )
			return NULL;

		memcpy( dst, program, len );
		dst[len] = '\0';
	}
	/* check for \ at beginning - get drive letter from cwd */
	else if( program[0] == '\\' || program[0] == '/' )
	{
		if( argv_len > (size_t)-1 - 2 )
			return NULL;
		len = 2 + argv_len;
		if( len >= capacity )
			return NULL;

		dst[0] = __fb_startup_cwd[0];
		dst[1] = ':';
		memcpy( dst + 2, program, argv_len );
		dst[len] = '\0';
	}
	/* no drive letter, no \, so relative path - get cur dir from startup */
	else
	{
		cwd_len = strlen(__fb_startup_cwd);
		if( (cwd_len > (size_t)-1 - 1) ||
		    (argv_len > (size_t)-1 - cwd_len - 1) )
			return NULL;

		len = cwd_len + 1 + argv_len;
		if( len >= capacity )
			return NULL;

		memcpy( dst, __fb_startup_cwd, cwd_len );
		dst[cwd_len] = '\\';
		memcpy( dst + cwd_len + 1, program, argv_len );
		dst[len] = '\0';
	}

	fb_hConvertPath( dst );

	p = strrchr( dst, '\\' );
	if( p != NULL )
		*p = '\0';

	/* upcase drive letter to be consistent with win32 port */
	dst[0] = (char)toupper((unsigned char)dst[0]);

	/* just a drive letter? make sure \ follows to prevent using relative path */
	if( capacity > 3 && dst[2] == '\0' && dst[1] == ':' &&
	    isalpha((unsigned char)dst[0]) )
	{
		dst[2] = '\\';
		dst[3] = '\0';
	}

	return p;
}

/* end of drivers/dos/sys_getexepath.c */
