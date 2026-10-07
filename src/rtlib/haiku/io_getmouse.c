/*
    FreeBASIC Haiku runtime
    File: io_getmouse.c
    Purpose: Return a defined unsupported result for console mouse queries.
    Responsibilities: Initialize optional outputs and report the runtime error.
    This file contains no graphics mouse polling or native window integration.
*/

#ifndef DISABLE_HAIKU

#include "../fb.h"

int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
    /* Graphics hooks dispatch separately. Calling fb_GetMouse here recurses. */
    if( x ) *x = -1;
    if( y ) *y = -1;
    if( z ) *z = -1;
    if( buttons ) *buttons = -1;
    if( clip ) *clip = -1;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

#endif

/* end of io_getmouse.c */
