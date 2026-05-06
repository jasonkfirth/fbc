/* mkdir function */

#include "../fb.h"
#include <direct.h>

FBCALL int fb_MkDir( FBSTRING *path )
{
	int res;

	res = _mkdir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
