/*
 * Project: FreeBASIC runtime library
 * File: files/file_lineinp_wstr.c
 * Purpose: line input function for wstrings.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_FileLineInputWstr( int fnum, FB_WCHAR *dst, ssize_t max_chars )
{
    FB_FILE *handle = FB_FILE_TO_HANDLE(fnum);

    if( !FB_HANDLE_USED(handle) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    if( handle->hooks->pfnReadLineWstr == NULL )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    return handle->hooks->pfnReadLineWstr( handle, dst, max_chars );
}

/* end of files/file_lineinp_wstr.c */
