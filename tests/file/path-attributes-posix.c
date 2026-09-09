/*
    FreeBASIC Runtime Tests
    File: file/path-attributes-posix.c
    Purpose: Test the POSIX pathname attribute adapter against real metadata.
    Responsibilities: Permission preservation, symlinks, and private cleanup.
    This file does not emulate POSIX chmod using Windows attribute bits.
*/

#include "../../src/rtlib/fb.h"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

static int last_error;

FBCALL int fb_ErrorSetNum( int error )
{
	last_error = error;
	return error;
}

int main( void )
{
	char directory[] = "/tmp/fb-attributes-XXXXXX";
	char filename[128], linkname[128], hidden[128];
	struct stat info;
	FILE *file;

	assert( mkdtemp( directory ) != NULL );
	assert( snprintf( filename, sizeof(filename), "%s/file", directory ) < (int)sizeof(filename) );
	assert( snprintf( linkname, sizeof(linkname), "%s/link", directory ) < (int)sizeof(linkname) );
	assert( snprintf( hidden, sizeof(hidden), "%s/.hidden", directory ) < (int)sizeof(hidden) );
	file = fopen( filename, "wb" );
	assert( file != NULL );
	assert( fputs( "unchanged", file ) >= 0 );
	assert( fclose( file ) == 0 );
	assert( chmod( filename, 0664 ) == 0 );
	assert( fb_FileSetAttr( filename, FB_FILE_ATTR_READONLY ) == 0 );
	assert( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0444 );
	assert( fb_FileGetAttr( filename ) == (FB_FILE_ATTR_READONLY | FB_FILE_ATTR_ARCHIVE) );
	assert( fb_FileSetAttr( filename, 0 ) == 0 );
	assert( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0644 );
	assert( info.st_size == 9 );
	assert( fb_FileSetAttr( filename, FB_FILE_ATTR_HIDDEN ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	assert( stat( filename, &info ) == 0 && (info.st_mode & 0777) == 0644 );
	assert( fb_FileGetAttr( directory ) & FB_FILE_ATTR_DIRECTORY );
	assert( symlink( filename, linkname ) == 0 );
	assert( lstat( linkname, &info ) == 0 && S_ISLNK( info.st_mode ) );
	assert( fb_FileGetAttr( linkname ) == fb_FileGetAttr( filename ) );
	assert( mkdir( hidden, 0700 ) == 0 );
	assert( fb_FileGetAttr( hidden ) == (FB_FILE_ATTR_DIRECTORY | FB_FILE_ATTR_HIDDEN) );
	strcat( hidden, "/" );
	assert( fb_FileGetAttr( hidden ) == (FB_FILE_ATTR_DIRECTORY | FB_FILE_ATTR_HIDDEN) );
	assert( fb_FileGetAttr( "" ) == -1 && last_error == FB_RTERROR_ILLEGALFUNCTIONCALL );
	assert( fb_FileGetAttr( NULL ) == -1 && last_error == FB_RTERROR_ILLEGALFUNCTIONCALL );
	assert( fb_FileSetAttr( NULL, 0 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );

	/* Only names within the directory returned by mkdtemp are removed. */
	assert( unlink( filename ) == 0 );
	assert( fb_FileGetAttr( linkname ) == -1 && last_error == FB_RTERROR_FILENOTFOUND );
	assert( unlink( linkname ) == 0 );
	assert( rmdir( hidden ) == 0 );
	assert( rmdir( directory ) == 0 );
	puts( "POSIX pathname attribute checks passed" );
	return 0;
}

/* end of file/path-attributes-posix.c */
