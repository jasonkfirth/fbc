/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_hconvpath.c

    Purpose:

        Normalize FreeBASIC file paths for AROS DOS and POSIXC services.

    Responsibilities:

        - translate backslashes to AROS-supported separators
        - remove redundant leading current-directory components
        - wrap AROS file, directory, and current-directory operations

    This file intentionally does NOT contain:

        - path rules for other operating systems
        - AROS filesystem or volume discovery
        - executable or shared-library search policy

    AROS path note:

        AROS POSIXC can read an existing path beginning with "./", but the
        underlying DOS handler may reject creation through that spelling.
        Native relative paths already start at the current directory, so the
        redundant prefix is removed before every operation.
*/

#include "../fb.h"

#include <sys/stat.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Path normalization                                                        */
/* ------------------------------------------------------------------------- */

void fb_hConvertPath( char *path )
{
	char *source;
	ssize_t index;
	ssize_t length;

	DBG_ASSERT( path != NULL );

	source = path;
	while( (source[0] == '.') && (source[1] == '/') )
		source += 2;

	if( source != path )
		memmove( path, source, strlen( source ) + 1 );

	length = strlen( path );
	for( index = 0; index < length; ++index )
	{
		if( path[index] == '\\' )
			path[index] = '/';
	}
}

static char *fb_hArosCopyPath( const char *path )
{
	char *copy;
	size_t length;

	DBG_ASSERT( path != NULL );

	length = strlen( path ) + 1;
	copy = malloc( length );
	if( copy == NULL )
		return NULL;

	memcpy( copy, path, length );
	fb_hConvertPath( copy );
	return copy;
}

/* ------------------------------------------------------------------------- */
/* C runtime wrappers                                                        */
/* ------------------------------------------------------------------------- */

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	char *converted_path;
	FILE *stream;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return NULL;

	stream = fopen( converted_path, mode );
	free( converted_path );
	return stream;
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	char *converted_path;
	FILE *result;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return NULL;

	result = freopen( converted_path, mode, stream );
	free( converted_path );
	return result;
}

int fb_hRemoveFile( const char *path )
{
	char *converted_path;
	int result;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return -1;

	result = remove( converted_path );
	free( converted_path );
	return result;
}

int fb_hMakeDir( const char *path )
{
	char *converted_path;
	int result;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return -1;

	result = mkdir( converted_path,
		S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
	free( converted_path );
	return result;
}

int fb_hChangeDir( const char *path )
{
	char *converted_path;
	int result;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return -1;

	result = chdir( converted_path );
	free( converted_path );
	return result;
}

int fb_hRemoveDir( const char *path )
{
	char *converted_path;
	int result;

	converted_path = fb_hArosCopyPath( path );
	if( converted_path == NULL )
		return -1;

	result = rmdir( converted_path );
	free( converted_path );
	return result;
}

/* end of aros/file_hconvpath.c */
