/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_cmd.c
 * Purpose: Provide sys cmd runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    DOS COMMAND$ implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* command$ */

#include "../../fb.h"

FBCALL FBSTRING_ACCESS *fb_Command ( int arg )
{
	FBSTRING_ACCESS *dst;
	size_t i, length;

	/* return all arguments? */
	if( arg < 0 )
	{
		char *p;

		/* no args? */
		if( (fb_ctx.argv == NULL) || (fb_ctx.argc <= 1) )
			return &fb_ctx.null_desc;

		/* concatenate all args but 0 */
		length = 0;
		for( i = 1; i < (size_t)fb_ctx.argc; i++ )
		{
			size_t arg_length = strlen( fb_ctx.argv[i] );
			if( arg_length > (size_t)FB_STRSIZEMSK - length )
				return &fb_ctx.null_desc;
			length += arg_length;
			if( i + 1 < (size_t)fb_ctx.argc )
			{
				if( length == (size_t)FB_STRSIZEMSK )
					return &fb_ctx.null_desc;
				length++;
			}
		}

		dst = fb_hStrAllocTemp( NULL, (ssize_t)length );
		if( dst == NULL )
			return &fb_ctx.null_desc;

		p = dst->data;
		for( i = 1; i < (size_t)fb_ctx.argc; i++ )
		{
			size_t arg_len = strlen( fb_ctx.argv[i] );
			memcpy( p, fb_ctx.argv[i], arg_len );
			p += arg_len;
			if( i + 1 < (size_t)fb_ctx.argc )
				*p++ = ' ';
		}
		*p = '\0';

		return dst;
	}

    /* return just one argument */
	if( arg >= fb_ctx.argc )
	    return &fb_ctx.null_desc;

	if( fb_ctx.argv == NULL )
		return &fb_ctx.null_desc;
	length = strlen( fb_ctx.argv[arg] );
	if( length > (size_t)FB_STRSIZEMSK )
		return &fb_ctx.null_desc;
	dst = fb_hStrAllocTemp( NULL, (ssize_t)length );
	if( dst == NULL )
		return &fb_ctx.null_desc;

	memcpy( dst->data, fb_ctx.argv[arg], length + 1 );

	if( arg == 0 )
	{
		/* make drive letter uppercase */
		if( (length >= 2) && (dst->data[1] == ':') )
			dst->data[0] = (char)toupper((unsigned char)dst->data[0]);

		/* DOS gives us argv[0] with '/' path separators -
		 * change them to the more DOS-like '\'. */
		for( i = 0; i < length; ++i )
		{
			if( dst->data[i] == '/' )
				dst->data[i] = '\\';
		}
	}

	return dst;
}

/* end of drivers/dos/sys_cmd.c */
