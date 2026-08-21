/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_size.c

    Purpose:

        Report a file's logical length without consulting stale AROS handler
        metadata after SetFileSize().

    Responsibilities:

        - validate the FreeBASIC file handle
        - return the size maintained by the AROS file-device layer
        - retain the generic seek-and-tell fallback for other devices

    This file intentionally does NOT contain:

        - AROS filesystem mutation
        - generic operating-system behavior
        - architecture policy

    AROS filesystem behavior:

        The FAT handler writes a shortened directory entry but leaves the
        size cached by an already-open file handle unchanged.  FreeBASIC's
        AROS file device therefore maintains handle->size as the authoritative
        logical value after writes, extending seeks, and FileSetEof.
*/

#include "../fb.h"

fb_off_t fb_FileSizeEx( FB_FILE *handle )
{
	fb_off_t result = 0;

	if( !FB_HANDLE_USED( handle ) )
		return result;

	FB_LOCK();

	if( handle->size >= 0 )
	{
		result = handle->size;
	}
	else if( handle->hooks->pfnSeek != NULL &&
		 handle->hooks->pfnTell != NULL )
	{
		fb_off_t old_position;
		int status;

		status = handle->hooks->pfnTell( handle, &old_position );
		if( status == 0 )
			status = handle->hooks->pfnSeek( handle, 0, SEEK_END );
		if( status == 0 )
		{
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

/* end of aros/file_size.c */
