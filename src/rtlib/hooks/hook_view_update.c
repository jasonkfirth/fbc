/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_view_update.c
 * Purpose: call back function that gets called whenever VIEW PRINT was called.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_ViewUpdate( void )
{
    FB_LOCK();

    if( __fb_ctx.hooks.viewupdateproc ) {
        __fb_ctx.hooks.viewupdateproc( );
    } else {
        fb_ConsoleViewUpdate( );
    }

    FB_UNLOCK();
}

/* end of hooks/hook_view_update.c */
