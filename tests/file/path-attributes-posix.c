/*
    FreeBASIC Runtime Tests
    File: file/path-attributes-posix.c
    Purpose: Test the POSIX pathname attribute adapter against real metadata.
    Responsibilities: Permission preservation, symlinks, and private cleanup.
    This file does not emulate POSIX chmod using Windows attribute bits.
*/

#include "../../src/rtlib/fb.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int last_error;

FBCALL int fb_ErrorSetNum( int error )
{
	last_error = error;
	return error;
}

static void require( int condition )
{
	if( !condition )
		abort();
}

int main( void )
{
	char directory[] = "/tmp/fb-attributes-XXXXXX";
	char filename[128], linkname[128], hidden[128];
	struct stat info;
	FILE *file;
	int close_result;

	require( mkdtemp( directory ) != NULL );
	require( snprintf( filename, sizeof(filename), "%s/file", directory ) < (int)sizeof(filename) );
	require( snprintf( linkname, sizeof(linkname), "%s/link", directory ) < (int)sizeof(linkname) );
	require( snprintf( hidden, sizeof(hidden), "%s/.hidden", directory ) < (int)sizeof(hidden) );
	file = fopen( filename, "wb" );
	require( file != NULL );
	require( fputs( "unchanged", file ) >= 0 );
	close_result = fclose( file );
	require( close_result == 0 );
	require( chmod( filename, 0664 ) == 0 );
	require( fb_FileSetAttr( filename, FB_FILE_ATTR_READONLY ) == 0 );
	require( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0444 );
	require( fb_FileGetAttr( filename ) == (FB_FILE_ATTR_READONLY | FB_FILE_ATTR_ARCHIVE) );
	require( fb_FileSetAttr( filename, 0 ) == 0 );
	require( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0644 );
	require( info.st_size == 9 );
	require( fb_FileSetAttr( filename, FB_FILE_ATTR_HIDDEN ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	require( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0644 );
	require( fb_FileGetAttr( directory ) & FB_FILE_ATTR_DIRECTORY );
	require( symlink( filename, linkname ) == 0 );
	require( lstat( linkname, &info ) == 0 && S_ISLNK( info.st_mode ) );
	require( fb_FileGetAttr( linkname ) == fb_FileGetAttr( filename ) );
	require( mkdir( hidden, 0700 ) == 0 );
	require( fb_FileGetAttr( hidden ) == (FB_FILE_ATTR_DIRECTORY | FB_FILE_ATTR_HIDDEN) );
	strcat( hidden, "/" );
	require( fb_FileGetAttr( hidden ) == (FB_FILE_ATTR_DIRECTORY | FB_FILE_ATTR_HIDDEN) );
	require( fb_FileGetAttr( "" ) == -1 && last_error == FB_RTERROR_ILLEGALFUNCTIONCALL );
	require( fb_FileGetAttr( NULL ) == -1 && last_error == FB_RTERROR_ILLEGALFUNCTIONCALL );
	require( fb_FileSetAttr( NULL, 0 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );

	/* Only names within the directory returned by mkdtemp are removed. */
	require( unlink( filename ) == 0 );
	require( fb_FileGetAttr( linkname ) == -1 && last_error == FB_RTERROR_FILENOTFOUND );
	require( unlink( linkname ) == 0 );
	require( rmdir( hidden ) == 0 );
	require( rmdir( directory ) == 0 );
	puts( "POSIX pathname attribute checks passed" );
	return 0;
}

/* end of file/path-attributes-posix.c */
