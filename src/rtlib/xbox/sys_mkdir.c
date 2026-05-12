/* mkdir function */

#include "../fb.h"

FBCALL int fb_MkDir( FBSTRING *path )
{
	int res;

	res = fb_hMakeDir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
