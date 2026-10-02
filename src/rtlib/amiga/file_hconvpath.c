/*
    FreeBASIC runtime library
    -------------------------

    File: amiga/file_hconvpath.c

    Purpose:

        Normalize FreeBASIC file paths for native AmigaOS DOS services.

    Responsibilities:

        - translate backslashes to AmigaOS-supported separators
        - remove redundant leading current-directory components
        - wrap AmigaOS file, directory, and current-directory operations

    This file intentionally does NOT contain:

        - path rules for other operating systems
        - AmigaOS filesystem or volume discovery
        - executable or shared-library search policy

    AmigaOS path note:

        DOS uses an empty component as a parent traversal: "../file" becomes
        "/file", and "dir/../file" becomes "dir//file". Existing native empty
        components must remain intact. Conversion only shortens the buffer.
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
	char *destination;
	int component_start;
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

	/* BASIC headers use Unix-style relative includes on every target. DOS
	   needs their dot components translated before the first existence check. */
	source = destination = path;
	component_start = TRUE;
	while( *source != '\0' ) {
		if( component_start && source[0] == '.' ) {
			if( source[1] == '/' || source[1] == '\0' ) {
				++source;
				if( *source == '/' ) ++source;
				else if( destination > path && destination[-1] == '/' ) --destination;
				continue;
			}
			if( source[1] == '.' && (source[2] == '/' || source[2] == '\0') ) {
				source += 2;
				if( *source == '/' ) ++source;
				*destination++ = '/';
				continue;
			}
		}
		component_start = (*source == '/' || *source == ':');
		*destination++ = *source++;
	}
	*destination = '\0';
}

static char *fb_hAmigaCopyPath( const char *path )
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

	converted_path = fb_hAmigaCopyPath( path );
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

	converted_path = fb_hAmigaCopyPath( path );
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

	converted_path = fb_hAmigaCopyPath( path );
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

	converted_path = fb_hAmigaCopyPath( path );
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

	converted_path = fb_hAmigaCopyPath( path );
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

	converted_path = fb_hAmigaCopyPath( path );
	if( converted_path == NULL )
		return -1;

	result = rmdir( converted_path );
	free( converted_path );
	return result;
}

/* end of amiga/file_hconvpath.c */
