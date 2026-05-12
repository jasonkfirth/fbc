/* chdir function */

#include "../fb.h"

FBCALL int fb_ChDir( FBSTRING *path )
{
	int res;

	res = fb_hChangeDir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
