/*
 * Project: FreeBASIC runtime library
 * File: console/io_widthfile.c
 * Purpose: set the with for files.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>

/*:::::*/
FBCALL int fb_WidthFile( int fnum, int width )
{
    int cur = width;
    FB_FILE *handle;

    if( width < -1 )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    FB_LOCK();

    handle = FB_HANDLE_DEREF(FB_FILE_TO_HANDLE(fnum));

    if( !FB_HANDLE_USED(handle) ) {
        /* invalid file handle */
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    if( handle->hooks==NULL ) {
        /* not opened yet */
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    if( handle==FB_HANDLE_SCREEN ) {
        /* SCREEN device */
        if( width!=-1 ) {
            fb_Width( width, -1 );
        }
        if( FB_HANDLE_SCREEN->width > INT_MAX ) {
            FB_UNLOCK();
            return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        }
        cur = (int)FB_HANDLE_SCREEN->width;

    } else {
        if( width!=-1 ) {
            handle->width = (unsigned int)width;
            if( handle->hooks->pfnSetWidth!=NULL )
                handle->hooks->pfnSetWidth( handle, width );
        }
        if( handle->width > INT_MAX ) {
            FB_UNLOCK();
            return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        }
        cur = (int)handle->width;
    }

	FB_UNLOCK();

    if( width==-1 ) {
        return cur;
    }

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of console/io_widthfile.c */
