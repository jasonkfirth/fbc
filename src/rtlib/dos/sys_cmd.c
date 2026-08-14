/*
    DOS COMMAND$ implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* command$ */

#include "../fb.h"

FBCALL FBSTRING *fb_Command ( int arg )
{
	FBSTRING *dst;
	ssize_t i, len;

	/* return all arguments? */
	if( arg < 0 )
	{
		char *p;

		/* no args? */
		if( __fb_ctx.argc <= 1 )
			return &__fb_ctx.null_desc;

		/* concatenate all args but 0 */
		len = 0;
		for( i = 1; i < __fb_ctx.argc; i++ )
			len += strlen( __fb_ctx.argv[i] );

		dst = fb_hStrAllocTemp( NULL, len + __fb_ctx.argc-2 );
		if( dst == NULL )
			return &__fb_ctx.null_desc;

		dst->data[0] = '\0';
		p = dst->data;
		for( i = 1; i < __fb_ctx.argc; i++ )
		{
			size_t arg_len = strlen( __fb_ctx.argv[i] );
			memcpy( p, __fb_ctx.argv[i], arg_len );
			p += arg_len;
			if( i != __fb_ctx.argc-1 )
				*p++ = ' ';
		}
		*p = '\0';

		return dst;
	}

    /* return just one argument */
	if( arg >= __fb_ctx.argc )
	    return &__fb_ctx.null_desc;

	len = strlen( __fb_ctx.argv[arg] );
	dst = fb_hStrAllocTemp( NULL, len );
	if( dst == NULL )
		return &__fb_ctx.null_desc;

	memcpy( dst->data, __fb_ctx.argv[arg], len + 1 );

	if( arg == 0 )
	{
		/* make drive letter uppercase */
		if( dst->data[1] == ':' )
			dst->data[0] = toupper( dst->data[0] );

		/* DOS gives us argv[0] with '/' path separators -
		 * change them to the more DOS-like '\'. */
		for( i = 0; i < len; ++i )
		{
			if( dst->data[i] == '/' )
				dst->data[i] = '\\';
		}
	}

	return dst;
}
