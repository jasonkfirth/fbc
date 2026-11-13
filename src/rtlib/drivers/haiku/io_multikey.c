/*
 * Project: FreeBASIC runtime library
 * File: drivers/haiku/io_multikey.c
 * Purpose: Haiku MULTIKEY runtime hook.
 * Responsibilities: Target-specific runtime support for haiku.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#ifndef DISABLE_HAIKU

#include "../../fb.h"
#include "../../../gfxlib2/fb_gfx.h"

int fb_ConsoleMultikey( int scancode )
{
    if( fb_gfx == NULL )
        return 0;

    if( fb_gfx->key == NULL )
        return 0;

    if( scancode < 0 || scancode >= 128 )
        return 0;

    return fb_gfx->key[scancode] ? 1 : 0;
}

#endif

/* end of drivers/haiku/io_multikey.c */
