/*
    FreeBASIC Runtime Library
    -------------------------

    File: file_hconvpath.c

    Purpose:

        Adapt FreeBASIC path strings to portable host filesystem interfaces.

    Responsibilities:

        - normalize directory separators
        - provide standard C and POSIX filesystem wrappers

    This file intentionally does NOT contain:

        - Windows ANSI or Unicode filesystem policy
        - directory enumeration
        - file-handle bookkeeping
*/

#include "fb.h"

#include <sys/stat.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Path separator conversion                                                 */
/* ------------------------------------------------------------------------- */

void fb_hConvertPath( char *path )
{
	ssize_t i, len;

	DBG_ASSERT( path != NULL );

	len = strlen( path );
	for( i = 0; i < len; i++ )
	{
#ifdef HOST_DOS
		if( path[i] == '/' )
			path[i] = '\\';
#else
		if( path[i] == '\\' )
			path[i] = '/';
#endif
	}
}

/* ------------------------------------------------------------------------- */
/* Portable filesystem operations                                            */
/* ------------------------------------------------------------------------- */

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	return fopen( path, mode );
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	return freopen( path, mode, stream );
}

int fb_hRemoveFile( const char *path )
{
	return remove( path );
}

int fb_hMakeDir( const char *path )
{
	return mkdir( path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
}

int fb_hChangeDir( const char *path )
{
	return chdir( path );
}

int fb_hRemoveDir( const char *path )
{
	return rmdir( path );
}

/* end of file_hconvpath.c */
