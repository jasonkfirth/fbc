/* get current dir */

#include "../fb.h"

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	if( fb_hGetExePath( dst, maxlen ) != NULL )
		return strlen( dst );

	*dst = '\0';
	return 0;
}
