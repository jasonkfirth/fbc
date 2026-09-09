/*
    FreeBASIC rtlib: win32/file_copy.c

    Copy a pathname through the native Windows service, using rtlib's UTF-8
    first path conversion. Windows owns alias checks and copy metadata.
    This file does not implement directory copies or file-content decoding.
*/

#ifdef __CYGWIN__
#include "../unix/file_copy.c"
#else

#include "../fb.h"
#include <windows.h>

FBCALL int fb_FileCopy( const char *source, const char *destination )
{
	BOOL res;
	char *source_ansi = NULL, *destination_ansi = NULL;
	wchar_t *source_wide, *destination_wide;

	if (!source || !*source || !destination || !*destination)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if (fb_hWin32IsWin9x()) {
		/* fb_hConvertPath changes separators in place for the Win9x ANSI APIs. */
		source_ansi = strdup(source);
		destination_ansi = strdup(destination);
		if (!source_ansi || !destination_ansi) {
			free(destination_ansi);
			free(source_ansi);
			return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		}
		fb_hConvertPath(source_ansi);
		fb_hConvertPath(destination_ansi);
		res = CopyFileA(source_ansi, destination_ansi, FALSE);
		free(destination_ansi);
		free(source_ansi);
	} else {
		source_wide = fb_hConvertPathToWC(source, NULL);
		if (!source_wide)
			return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		destination_wide = fb_hConvertPathToWC(destination, NULL);
		if (!destination_wide) {
			free(source_wide);
			return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		}
		res = CopyFileW(source_wide, destination_wide, FALSE);
		free(destination_wide);
		free(source_wide);
	}
	return fb_ErrorSetNum( res == FALSE ? FB_RTERROR_ILLEGALFUNCTIONCALL : FB_RTERROR_OK );
}

#endif

/* end of file_copy.c */
