/*
    Project: FreeBASIC classic AmigaOS runtime
    ---------------------------------------

    File: drivers/amiga/file_size.c

    Purpose:
        Query read-only file lengths without disturbing newlib's input buffer.

    Responsibilities:
        - use native descriptor metadata for ordinary read-only disk files
        - preserve the generic seek/tell behavior for writable/custom devices
        - serialize descriptor queries through the existing runtime lock

    This file intentionally does NOT contain:
        - descriptor ownership or cursor mutation
        - file-size caching or filesystem implementation

    The generic LOF path seeks to EOF and back. Newlib can refill its input
    buffer while restoring that position, adding disk reads to every query.
    Native fstat examines the same open DOS handle without moving its cursor.
    Keep querying metadata instead of caching a size: another handle may have
    extended or truncated the file since it was opened.
*/

#include "../../fb.h"
#include <sys/stat.h>

fb_off_t fb_FileSizeEx( FB_FILE *handle )
{
	fb_off_t result = 0;

	if( !FB_HANDLE_USED( handle ) )
		return result;

	FB_LOCK();

	/* Ordinary OPEN files also carry the VFS type. Exact standard hooks
	   identify their FILE storage, and fstat must still confirm a disk file.
	   Writable streams can have pending stdio output. Keep their established
	   flushing/seek behavior, and never interpret a custom device as FILE. */
	if( (handle->type == FB_FILE_TYPE_NORMAL ||
	     handle->type == FB_FILE_TYPE_VFS) && handle->opaque != NULL &&
	    handle->hooks->pfnSeek == fb_DevFileSeek &&
	    handle->hooks->pfnTell == fb_DevFileTell &&
	    (handle->access == FB_FILE_ACCESS_READ ||
	     handle->mode == FB_FILE_MODE_INPUT) ) {
		struct stat information;
		FILE *stream = (FILE *)handle->opaque;
		int descriptor = fileno( stream );

		if( descriptor >= 0 && fstat( descriptor, &information ) == 0 &&
		    S_ISREG( information.st_mode ) && information.st_size >= 0 ) {
			result = (fb_off_t)information.st_size;
			fb_ErrorSetNum( FB_RTERROR_OK );
			FB_UNLOCK();
			return result;
		}
	}

	if( handle->hooks->pfnSeek != NULL && handle->hooks->pfnTell != NULL ) {
		fb_off_t old_position;
		int status = handle->hooks->pfnTell( handle, &old_position );

		if( status == 0 )
			status = handle->hooks->pfnSeek( handle, 0, SEEK_END );
		if( status == 0 ) {
			if( handle->hooks->pfnTell( handle, &result ) != 0 )
				result = 0;
			handle->hooks->pfnSeek( handle, old_position, SEEK_SET );
		}
	}

	FB_UNLOCK();
	return result;
}

FBCALL long long fb_FileSize( int file_number )
{
	return fb_FileSizeEx( FB_FILE_TO_HANDLE( file_number ) );
}

/* end of drivers/amiga/file_size.c */
