/*
    FreeBASIC Runtime Library
    File: file_rename.c
    Purpose: Rename files through the platform's Unicode path interface.
    Responsibilities: Preserve NAME's C rename result and UTF-8 path bytes.
    This file intentionally does NOT contain filename parsing or file handles.
*/

#include "fb.h"

/* NAME uses the C calling convention and retains rename()'s 0/-1 result.
   Desktop Windows needs wide paths; other providers already receive UTF-8
   bytes, including the Windows CE CRT adapter. Win9x retains its ANSI API. */
int fb_FileRename( const char *oldname, const char *newname )
{
#if defined(HOST_MINGW) && !defined(HOST_WINCE)
	wchar_t *oldpath, *newpath;
	int result;
	if( fb_hWin32IsWin9x() ) return rename(oldname, newname);
	oldpath = fb_hConvertPathToWC(oldname, NULL);
	newpath = fb_hConvertPathToWC(newname, NULL);
	if( oldpath == NULL || newpath == NULL ) result = -1;
	else result = _wrename(oldpath, newpath);
	free(oldpath);
	free(newpath);
	return result;
#else
	return rename(oldname, newname);
#endif
}

/* end of file_rename.c */
