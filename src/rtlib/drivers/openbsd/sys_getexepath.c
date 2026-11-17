/*
 * Project: FreeBASIC runtime library
 * File: drivers/openbsd/sys_getexepath.c
 * Purpose: get the executable path.
 * Responsibilities: Target-specific runtime support for openbsd.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	const char *p;
	size_t capacity;
	if( dst == NULL || maxlen <= 0 || fb_ctx.argv == NULL || fb_ctx.argv[0] == NULL )
		return NULL;

	capacity = (size_t)maxlen;
	p = strrchr( fb_ctx.argv[0], '/' );
	if( p ) {
		size_t length = (size_t)(p - fb_ctx.argv[0]);
		if( length == 0 ) {
		/* keep the "/" rather than returning "" */
			length = 1;
		}
		if( length >= capacity )
			length = capacity - 1;

		memcpy( dst, fb_ctx.argv[0], length );
		dst[length] = '\0';
	} else {
		const char *path = getenv( "PATH" );
		size_t argv_len = strlen( fb_ctx.argv[0] );

		while( path && *path ) {
			char candidate[MAX_PATH+1];
			const char *end = strchr( path, ':' );
			size_t path_len = end ? (size_t)(end - path) : strlen( path );

			if( path_len == 0 ) {
				path = end ? end + 1 : NULL;
				continue;
			}

			if( path_len < sizeof( candidate ) &&
			    argv_len < sizeof( candidate ) - path_len - 1 ) {
				memcpy( candidate, path, path_len );
				candidate[path_len] = '/';
				memcpy( candidate + path_len + 1, fb_ctx.argv[0], argv_len );
				candidate[path_len + 1 + argv_len] = '\0';

				if( access( candidate, X_OK ) == 0 ) {
					p = strrchr( candidate, '/' );
					if( p ) {
						size_t length = (size_t)(p - candidate);
						if( length >= capacity )
							length = capacity - 1;
						memcpy( dst, candidate, length );
						dst[length] = '\0';
						return dst;
					}
				}
			}

			path = end ? end + 1 : NULL;
		}

		*dst = '\0';
		return NULL;
	}
	return dst;
}

/* end of drivers/openbsd/sys_getexepath.c */
