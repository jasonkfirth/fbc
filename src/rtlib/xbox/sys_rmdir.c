/* rmdir function */

#include "../fb.h"

FBCALL int fb_RmDir( FBSTRING *path )
{
	int res;

	res = fb_hRemoveDir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}
