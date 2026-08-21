/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_seteof.c

    Purpose:

        Set a file's logical end and keep the runtime size cache coherent.

    Responsibilities:

        - validate modes accepted by FileSetEof
        - flush the FreeBASIC file device before resizing
        - invoke the AROS-local SetFileSize implementation
        - record the resulting logical size on the runtime handle

    This file intentionally does NOT contain:

        - filesystem-handler internals
        - generic operating-system behavior
        - architecture policy
*/

#include "../fb.h"

int fb_FileSetEofEx( FB_FILE *handle )
{
	fb_off_t position;
	int result;

	FB_LOCK();

	if( !FB_HANDLE_USED( handle ) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	switch( handle->mode )
	{
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
	case FB_FILE_MODE_OUTPUT:
	case FB_FILE_MODE_APPEND:
		break;

	default:
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	position = fb_hArosGetFilePosition( (FILE *)handle->opaque );
	if( position < 0 )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	result = fb_FileFlushEx( handle, FALSE );
	if( result == FB_RTERROR_OK )
		result = fb_hFileSetEofEx( (FILE *)handle->opaque );

	if( result == FB_RTERROR_OK )
		handle->size = position;

	FB_UNLOCK();
	return result;
}

FBCALL int fb_FileSetEof( int file_number )
{
	return fb_FileSetEofEx( FB_FILE_TO_HANDLE( file_number ) );
}

/* end of aros/file_seteof.c */
