/* rmdir function */

#include "../fb.h"
#include <direct.h>

FBCALL int fb_RmDir( FBSTRING *path )
{
	int res;

	res = _rmdir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
