/*
    FreeBASIC Runtime Library
    File: win32/file_attrs.c
    Purpose: Implement pathname attributes on desktop Windows.
    Responsibilities: Preserve native metadata and use runtime path encoding.
    This file does not enumerate directories or manage open file handles.
*/

#ifdef __CYGWIN__
/* Cygwin filenames and permissions belong to its POSIX filesystem view. */
#include "../unix/file_attrs.c"
#else

#include "../fb.h"
#include <windows.h>

static int attribute_error( DWORD error )
{
	if( error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ||
	    error == ERROR_INVALID_DRIVE )
		return FB_RTERROR_FILENOTFOUND;
	if( error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION ||
	    error == ERROR_WRITE_PROTECT )
		return FB_RTERROR_NOPRIVILEGES;
	return FB_RTERROR_FILEIO;
}

/* A negative requested value selects a query. Public SetAttr validates its
   mask before entering here. Convert the path once for the entire operation. */
static int file_attributes( const char *filename, int requested )
{
	wchar_t *wide_path = NULL;
	char *ansi_path = NULL;
	const char *pattern;
	DWORD attributes, native_error;
	int error = FB_RTERROR_OK, result = -1;
	BOOL success;

	if( filename == NULL || filename[0] == '\0' ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}
	/* The question mark in an extended-length path prefix is not a wildcard. */
	pattern = filename;
	if( strncmp( pattern, "\\\\?\\", 4 ) == 0 )
		pattern += 4;
	if( strchr( pattern, '*' ) || strchr( pattern, '?' ) ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}
	/* Windows 9x has no usable W filesystem APIs. NT uses the existing
	   UTF-8-first, system-codepage-fallback conversion shared by file I/O. */
	if( fb_hWin32IsWin9x() ) {
		ansi_path = strdup( filename );
		if( ansi_path )
			fb_hConvertPath( ansi_path );
	} else {
		wide_path = fb_hConvertPathToWC( filename, NULL );
	}
	if( ansi_path == NULL && wide_path == NULL ) {
		error = FB_RTERROR_OUTOFMEM;
		goto done;
	}
	attributes = wide_path ? GetFileAttributesW( wide_path ) : GetFileAttributesA( ansi_path );
	if( attributes == INVALID_FILE_ATTRIBUTES ) {
		error = attribute_error( GetLastError() );
		goto done;
	}
	if( requested < 0 ) {
		result = attributes & (FB_FILE_ATTR_SETTABLE | FB_FILE_ATTR_DIRECTORY);
		goto done;
	}

	/* Keep metadata outside VB's writable mask, including directory and
	   reparse-point identity. NORMAL is used only when no bits remain. */
	attributes &= ~(FB_FILE_ATTR_SETTABLE | FILE_ATTRIBUTE_NORMAL);
	attributes |= requested;
	if( attributes == 0 )
		attributes = FILE_ATTRIBUTE_NORMAL;
	success = wide_path ? SetFileAttributesW( wide_path, attributes ) :
	                      SetFileAttributesA( ansi_path, attributes );
	native_error = GetLastError();
	if( !success )
		error = attribute_error( native_error );

done:
	free( wide_path );
	free( ansi_path );
	fb_ErrorSetNum( error );
	return requested < 0 ? result : error;
}

FBCALL int fb_FileGetAttr( const char *filename )
{
	return file_attributes( filename, -1 );
}

FBCALL int fb_FileSetAttr( const char *filename, int attributes )
{
	if( (attributes & ~FB_FILE_ATTR_SETTABLE) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return file_attributes( filename, attributes );
}

#endif

/* end of win32/file_attrs.c */
