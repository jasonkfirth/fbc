/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_sleep.c
 * Purpose: sleep function.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_Sleep ( int msecs )
{
    FB_SLEEPPROC sleepproc;
    FB_LOCK();
    sleepproc = __fb_ctx.hooks.sleepproc;
    FB_UNLOCK();
    if( sleepproc ) {
        sleepproc( msecs );
    } else {
        fb_ConsoleSleep( msecs );
    }
}

/* end of hooks/hook_sleep.c */
