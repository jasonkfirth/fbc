/* chdir function */

#include "../fb.h"
#include <direct.h>

FBCALL int fb_ChDir( FBSTRING *path )
{
	int res;

	res = _chdir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
