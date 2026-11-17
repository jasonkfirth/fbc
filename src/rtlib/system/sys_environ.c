/*
 * Project: FreeBASIC runtime library
 * File: system/sys_environ.c
 * Purpose: environ$ function and setenviron stmt.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

#ifndef HOST_MINGW
static int hSetEnviron( const char *text )
{
#ifdef HOST_DOS
	char *copy;
	size_t len;
	int res;

	len = strlen( text );
	if( len == SIZE_MAX )
		return -1;
	++len;
	copy = (char *)malloc( len );
	if( copy == NULL )
		return -1;

	memcpy( copy, text, len );

	/*
	 * DJGPP has putenv(), but POSIX putenv() keeps this buffer instead of
	 * copying it.  Leave it allocated on success so getenv() can keep using
	 * the string.
	 */
	res = putenv( copy );
	if( res != 0 )
		free( copy );

	return res;
#else
	const char *equals;
	char *name;
	size_t name_len;
	int res;

	equals = strchr( text, '=' );
	if( equals == NULL )
		return unsetenv( text );

	name_len = (size_t)(equals - text);
	if( name_len == 0 )
		return -1;
	if( name_len == SIZE_MAX )
		return -1;

	name = (char *)malloc( name_len + 1 );
	if( name == NULL )
		return -1;

	memcpy( name, text, name_len );
	name[name_len] = '\0';

	res = setenv( name, equals + 1, 1 );
	free( name );

	return res;
#endif
}
#endif

FBCALL FBSTRING_ACCESS *fb_GetEnviron ( FBSTRING_ACCESS *varname )
{
	FBSTRING_ACCESS 	*dst;
	char 		*tmp;
	size_t len;

	if( (varname != NULL) && (varname->data != NULL) )
		tmp = getenv( varname->data );
	else
		tmp = NULL;

	FB_STRLOCK();

	if( tmp != NULL )
	{
        len = strlen( tmp );
		if( len > (size_t)FB_STRSIZEMSK )
			dst = &fb_ctx.null_desc;
		else {
			dst = fb_hStrAllocTemp_NoLock( NULL, (ssize_t)len );
			if( dst != NULL )
				fb_hStrCopy( dst->data, tmp, (ssize_t)len );
			else
				dst = &fb_ctx.null_desc;
		}
	}
	else
		dst = &fb_ctx.null_desc;

	/* del if temp */
	fb_hStrDelTemp_NoLock( varname );

	FB_STRUNLOCK();

	return dst;
}

FBCALL int fb_SetEnviron ( FBSTRING_ACCESS *str )
{
	int res = 0;

	if( (str != NULL) && (str->data != NULL) )
	{
#ifdef HOST_MINGW
		res = _putenv( str->data );
#else
		res = hSetEnviron( str->data );
#endif
	}

	/* del if temp */
	fb_hStrDelTemp( str );

	return res;
}

/* end of system/sys_environ.c */
