/*
 * Project: FreeBASIC runtime library
 * File: files/file_openerr.c
 * Purpose: open ERR.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_FileOpenErr ( FBSTRING *str_filename, unsigned int mode,
                            unsigned int access, unsigned int lock,
                            int fnum, int len, const char *encoding )
{
    if( !FB_FILE_INDEX_VALID( fnum ) )
    	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    return fb_FileOpenVfsEx( FB_FILE_TO_HANDLE(fnum),
                             str_filename,
                             mode,
                             access,
                             lock,
                             len,
                             fb_hFileStrToEncoding( encoding ),
                             fb_DevErrOpen );
}

/* end of files/file_openerr.c */
