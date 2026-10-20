/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_readxy.c
 * Purpose: reads color valoe or character from X/Y position.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL unsigned int fb_ReadXY( int col, int row, int colorflag )
{
    unsigned int res;

    FB_LOCK();

    if( __fb_ctx.hooks.readxyproc ) {
        res = __fb_ctx.hooks.readxyproc( col, row, colorflag );
    } else {
        res = fb_ConsoleReadXY( col, row, colorflag );
    }

    FB_UNLOCK();

	return res;
}

/* end of hooks/hook_readxy.c */
