/* get the executable's name */

#include "../fb.h"
#include <string.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	const char *name = "D:\\default.xbe";
	ssize_t len = strlen( name );

	if( maxlen <= 0 )
		return dst;

	if( len >= maxlen )
		len = maxlen - 1;

	memcpy( dst, name, len );
	dst[len] = '\0';

	return dst;
}
