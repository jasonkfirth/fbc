/* environ$ function and setenviron stmt */

#include "fb.h"

#ifndef HOST_MINGW
static int hSetEnviron( const char *text )
{
#ifdef HOST_DOS
	char *copy;
	ssize_t len;
	int res;

	len = strlen( text ) + 1;
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
	ssize_t name_len;
	int res;

	equals = strchr( text, '=' );
	if( equals == NULL )
		return unsetenv( text );

	name_len = equals - text;
	if( name_len <= 0 )
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

FBCALL FBSTRING *fb_GetEnviron ( FBSTRING *varname )
{
	FBSTRING 	*dst;
	char 		*tmp;
	ssize_t len;

	if( (varname != NULL) && (varname->data != NULL) )
		tmp = getenv( varname->data );
	else
		tmp = NULL;

	FB_STRLOCK();

	if( tmp != NULL )
	{
        len = strlen( tmp );
        dst = fb_hStrAllocTemp_NoLock( NULL, len );
		if( dst != NULL )
		{
			fb_hStrCopy( dst->data, tmp, len );
		}
		else
			dst = &__fb_ctx.null_desc;
	}
	else
		dst = &__fb_ctx.null_desc;

	/* del if temp */
	fb_hStrDelTemp_NoLock( varname );

	FB_STRUNLOCK();

	return dst;
}

FBCALL int fb_SetEnviron ( FBSTRING *str )
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
