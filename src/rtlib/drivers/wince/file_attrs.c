/*
    Project: FreeBASIC Runtime Library
    File: drivers/wince/file_attrs.c
    Purpose: Implement pathname attributes through Windows CE file APIs.
    Responsibilities: Query and set supported file attributes using UTF-16 paths.
    This file intentionally does NOT contain: directory enumeration or POSIX mode mapping.
*/

#include "../../fb.h"

#include <windows.h>
#include <string.h>

static int attribute_error( DWORD error )
{
	if( error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ||
	    error == ERROR_INVALID_NAME )
		return FB_RTERROR_FILENOTFOUND;
	if( error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION ||
	    error == ERROR_WRITE_PROTECT )
		return FB_RTERROR_NOPRIVILEGES;
	return FB_RTERROR_FILEIO;
}

static int file_attributes( const char *filename, int requested )
{
	wchar_t *wide_path = NULL;
	DWORD attributes, native_error;
	int error = FB_RTERROR_OK;
	int result = -1;
	BOOL success;

	if( filename == NULL || filename[0] == '\0' ||
	    strchr( filename, '*' ) != NULL || strchr( filename, '?' ) != NULL ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}

	wide_path = fb_hConvertPathToWC( filename, NULL );
	if( wide_path == NULL ) {
		error = FB_RTERROR_OUTOFMEM;
		goto done;
	}

	SetLastError( ERROR_SUCCESS );
	attributes = GetFileAttributesW( wide_path );
	if( attributes == INVALID_FILE_ATTRIBUTES ) {
		error = attribute_error( GetLastError() );
		goto done;
	}

	if( requested < 0 ) {
		result = attributes & (FB_FILE_ATTR_SETTABLE | FB_FILE_ATTR_DIRECTORY);
		goto done;
	}

	attributes &= ~((DWORD)FB_FILE_ATTR_SETTABLE | (DWORD)FILE_ATTRIBUTE_NORMAL);
	attributes |= (DWORD)requested;
	if( attributes == 0 )
		attributes = FILE_ATTRIBUTE_NORMAL;

	SetLastError( ERROR_SUCCESS );
	success = SetFileAttributesW( wide_path, attributes );
	native_error = GetLastError();
	if( !success )
		error = attribute_error( native_error );

done:
	free( wide_path );
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

/* end of drivers/wince/file_attrs.c */
