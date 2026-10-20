/*
 * Project: FreeBASIC runtime library
 * File: files/file_input_file.c
 * Purpose: input function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_FileInput( int fnum )
{
    FB_INPUTCTX *ctx;
    FB_FILE *handle = NULL;

    FB_LOCK();

    handle = FB_FILE_TO_HANDLE(fnum);
    if( !FB_HANDLE_USED(handle) )
    {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    ctx = FB_TLSGETCTX( INPUT );

    ctx->handle = handle;
    ctx->status = 0;
    fb_StrDelete( &ctx->str );
    ctx->index	= 0;

    FB_UNLOCK();

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

void fb_INPUTCTX_Destructor( void* data )
{
    FB_INPUTCTX *ctx = (FB_INPUTCTX *)data;
    fb_StrDelete( &ctx->str );
    /* The file handle is closed by the program, it's not ours to clean up */
}

/* end of files/file_input_file.c */
