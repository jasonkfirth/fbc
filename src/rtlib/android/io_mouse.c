/* console mode mouse functions */

#include "../fb.h"
#include "../../gfxlib2/fb_gfx.h"

int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
	return fb_GfxGetMouse( x, y, z, buttons, clip );
}

int fb_ConsoleSetMouse( int x, int y, int cursor, int clip )
{
	return fb_GfxSetMouse( x, y, cursor, clip );
}
