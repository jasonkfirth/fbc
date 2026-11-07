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
	ssize_t i, len;

	/* return all arguments? */
	if( arg < 0 )
	{
		char *p;

		/* no args? */
		if( fb_ctx.argc <= 1 )
			return &fb_ctx.null_desc;

		/* concatenate all args but 0 */
		len = 0;
		for( i = 1; i < fb_ctx.argc; i++ )
			len += strlen( fb_ctx.argv[i] );

		dst = fb_hStrAllocTemp( NULL, len + fb_ctx.argc-2 );
		if( dst == NULL )
			return &fb_ctx.null_desc;

		dst->data[0] = '\0';
		p = dst->data;
		for( i = 1; i < fb_ctx.argc; i++ )
		{
			size_t arg_len = strlen( fb_ctx.argv[i] );
			memcpy( p, fb_ctx.argv[i], arg_len );
			p += arg_len;
			if( i != fb_ctx.argc-1 )
				*p++ = ' ';
		}
		*p = '\0';

		return dst;
	}

    /* return just one argument */
	if( arg >= fb_ctx.argc )
	    return &fb_ctx.null_desc;

	len = strlen( fb_ctx.argv[arg] );
	dst = fb_hStrAllocTemp( NULL, len );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	memcpy( dst->data, fb_ctx.argv[arg], len + 1 );

	return dst;
}

/* end of system/sys_cmd.c */
