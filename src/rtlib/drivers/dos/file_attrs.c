/*
 * Project: FreeBASIC DOS runtime
 * File: drivers/dos/file_attrs.c
 * Purpose: Query and update DOS pathname attributes independently of DIR.
 * Responsibilities: Native attribute bits, drive roots and runtime errors.
 * This file intentionally does NOT contain: directory iteration or
 * working-directory selection, which belong to their own drivers.
 */

#include "../../fb.h"
#include <dos.h>
#include <errno.h>
#include <sys/stat.h>

static int attribute_error( void )
{
	if( errno == ENOENT || errno == ENOTDIR )
		return FB_RTERROR_FILENOTFOUND;
	if( errno == EACCES || errno == EPERM || errno == EROFS )
		return FB_RTERROR_NOPRIVILEGES;
	return FB_RTERROR_FILEIO;
}

static int attribute_path_valid( const char *filename )
{
	return filename != NULL && filename[0] != '\0' &&
	       strpbrk( filename, "*?" ) == NULL;
}

static int attribute_root( const char *filename )
{
	const char *cursor = filename;
	if( ((cursor[0] >= 'A' && cursor[0] <= 'Z') ||
	     (cursor[0] >= 'a' && cursor[0] <= 'z')) && cursor[1] == ':' )
		cursor += 2;
	if( *cursor != '/' && *cursor != '\\' )
		return FALSE;

	while( *cursor == '/' || *cursor == '\\' )
		++cursor;
	return *cursor == '\0';
}

FBCALL int fb_FileGetAttr( const char *filename )
{
	unsigned int attributes;
	int result, error;

	if( !attribute_path_valid( filename ) ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return -1;
	}

	/* DJGPP's DOS calls use its shared real-mode transfer area. Match the
	 * runtime's filesystem lock and leave the caller's DIR cursor untouched.
	 * DOS can reject root attribute queries. stat verifies the drive exists
	 * before reporting the directory bit for that special case.
	 */
	FB_LOCK();
	result = _dos_getfileattr( filename, &attributes );
	if( result != 0 && attribute_root( filename ) ) {
		struct stat information;
		if( stat( filename, &information ) == 0 && S_ISDIR( information.st_mode ) ) {
			attributes = FB_FILE_ATTR_DIRECTORY;
			result = 0;
		}
	}
	error = result == 0 ? FB_RTERROR_OK : attribute_error();
	FB_UNLOCK();
	fb_ErrorSetNum( error );
	return result == 0 ? (int)attributes : -1;
}

FBCALL int fb_FileSetAttr( const char *filename, int attributes )
{
	unsigned int existing;
	int result, error;

	if( !attribute_path_valid( filename ) ||
	    (attributes & ~FB_FILE_ATTR_SETTABLE) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	/* Preserve directory and other structural bits from the native query.
	 * Query and update share one runtime exclusion interval.
	 */
	FB_LOCK();
	result = _dos_getfileattr( filename, &existing );
	if( result == 0 )
		result = _dos_setfileattr( filename,
			(existing & ~FB_FILE_ATTR_SETTABLE) | (unsigned int)attributes );
	error = result == 0 ? FB_RTERROR_OK : attribute_error();
	FB_UNLOCK();
	return fb_ErrorSetNum( error );
}

/* end of drivers/dos/file_attrs.c */
