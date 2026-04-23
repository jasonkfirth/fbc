/* Haiku MULTIKEY runtime hook */

#ifndef DISABLE_HAIKU

#include "../fb.h"
#include "../../gfxlib2/fb_gfx.h"

int fb_ConsoleMultikey( int scancode )
{
    if( __fb_gfx == NULL )
        return 0;

    if( __fb_gfx->key == NULL )
        return 0;

    if( scancode < 0 || scancode >= 128 )
        return 0;

    return __fb_gfx->key[scancode] ? 1 : 0;
}

#endif
