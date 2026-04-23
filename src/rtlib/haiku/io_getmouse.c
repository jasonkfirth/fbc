/* Haiku GETMOUSE runtime hook */

#ifndef DISABLE_HAIKU

#include "../fb.h"

int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
    return fb_GetMouse( x, y, z, buttons, clip );
}

#endif
