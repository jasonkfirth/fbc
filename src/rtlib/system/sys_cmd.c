/*
 * Project: FreeBASIC runtime library
 * File: system/sys_cmd.c
 * Purpose: command$.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_Command ( int arg )
{
	FBSTRING_ACCESS *dst;
	size_t argc, i, len;
	size_t arg_index;
	const size_t max_len = (size_t)FB_STRSIZEMSK;

	/* return all arguments? */
	if( arg < 0 )
	{
		char *p;

		/* no args? */
		if( fb_ctx.argc <= 1 )
			return &fb_ctx.null_desc;

		/* concatenate all args but 0 */
		argc = (size_t)fb_ctx.argc;
		len = 0;
		for( i = 1; i < argc; i++ )
		{
			size_t arg_len = strlen( fb_ctx.argv[i] );
			if( arg_len > max_len - len )
				return &fb_ctx.null_desc;
			len += arg_len;
		}
		if( argc - 2 > max_len - len )
			return &fb_ctx.null_desc;
		len += argc - 2;

		dst = fb_hStrAllocTemp( NULL, (ssize_t)len );
		if( dst == NULL )
			return &fb_ctx.null_desc;

		dst->data[0] = '\0';
		p = dst->data;
		for( i = 1; i < argc; i++ )
		{
			size_t arg_len = strlen( fb_ctx.argv[i] );
			memcpy( p, fb_ctx.argv[i], arg_len );
			p += arg_len;
			if( i != argc-1 )
				*p++ = ' ';
		}
		*p = '\0';

		return dst;
	}

    /* return just one argument */
	if( arg >= fb_ctx.argc )
	    return &fb_ctx.null_desc;

	arg_index = (size_t)arg;
	len = strlen( fb_ctx.argv[arg_index] );
	if( len > max_len )
		return &fb_ctx.null_desc;
	dst = fb_hStrAllocTemp( NULL, (ssize_t)len );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	memcpy( dst->data, fb_ctx.argv[arg_index], len + 1 );

	return dst;
}

/* end of system/sys_cmd.c */
