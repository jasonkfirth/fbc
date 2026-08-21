/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_copy.c

    Purpose:

        Implement FILECOPY with native Windows CE Unicode paths.

    Responsibilities:

        - convert both FreeBASIC path strings to UTF-16
        - preserve Windows FILECOPY overwrite behavior
        - translate API failure to a FreeBASIC runtime error

    This file intentionally does NOT contain:

        - ANSI Windows API fallbacks
        - recursive directory copying
        - metadata normalization
*/

#include "../fb.h"

#include <windows.h>

FBCALL int fb_FileCopy( const char *source, const char *destination )
{
	wchar_t *wide_source;
	wchar_t *wide_destination;
	BOOL copied = FALSE;

	if( (source == NULL) || (destination == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	wide_source = fb_hConvertPathToWC( source, NULL );
	wide_destination = fb_hConvertPathToWC( destination, NULL );

	if( (wide_source != NULL) && (wide_destination != NULL) ) {
		copied = CopyFileW( wide_source, wide_destination, FALSE );
	}

	free( wide_source );
	free( wide_destination );

	return fb_ErrorSetNum( copied ? FB_RTERROR_OK :
	                                FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of wince/file_copy.c */
