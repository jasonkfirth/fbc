/*
 * Project: FreeBASIC runtime library
 * File: files/file_openencod.c
 * Purpose: UTF-encoded file open function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_FileOpenEncod
	(
		FBSTRING *str,
		unsigned int mode,
		unsigned int access,
		unsigned int lock,
		int fnum,
		int len,
		const char *encoding
	)
{
    if( !FB_FILE_INDEX_VALID( fnum ) )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    FB_FILE_ENCOD encod = fb_hFileStrToEncoding( encoding );

    return fb_FileOpenVfsEx( FB_FILE_TO_HANDLE(fnum), str, mode,
                             access, lock, len, encod,
                             (encod == FB_FILE_ENCOD_ASCII?
                              fb_DevFileOpen :
                              fb_DevFileOpenEncod) );
}

/* end of files/file_openencod.c */
