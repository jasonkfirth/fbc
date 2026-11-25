/*
    Project: FreeBASIC Runtime Library
    File: files/file_rename.c
    Purpose: Rename files through the platform's Unicode path interface.
    Responsibilities: Preserve NAME's C rename result and UTF-8 path bytes.
    This file intentionally does NOT contain: filename parsing or file handles.
*/

#include "fb.h"
#ifdef HOST_XBOX
#include "drivers/xbox/file_path.h"
#endif

/* NAME uses the C calling convention and retains rename()'s 0/-1 result.
   Desktop Windows needs wide paths; other providers already receive UTF-8
   bytes, including the Windows CE CRT adapter. Win9x retains its ANSI API. */
int fb_FileRename( const char *oldname, const char *newname )
{
#if defined(HOST_MINGW) && !defined(HOST_WINCE)
	FB_WCHAR *oldpath, *newpath;
	char *old_ansi, *new_ansi;
	int result;
	if( fb_hWin32IsWin9x() ) {
		old_ansi = fb_hConvertPathToAC( oldname );
		new_ansi = fb_hConvertPathToAC( newname );
		if( old_ansi == NULL || new_ansi == NULL ) {
			free( old_ansi );
			free( new_ansi );
			return -1;
		}
		result = rename( old_ansi, new_ansi );
		free( old_ansi );
		free( new_ansi );
		return result;
	}
	oldpath = fb_hConvertPathToWC(oldname, NULL);
	newpath = fb_hConvertPathToWC(newname, NULL);
	if( oldpath == NULL || newpath == NULL ) result = -1;
	else result = _wrename(oldpath, newpath);
	free(oldpath);
	free(newpath);
	return result;
#elif defined(HOST_XBOX)
	return fb_hRenameFile(oldname, newname);
#else
	return rename(oldname, newname);
#endif
}

/* end of files/file_rename.c */
