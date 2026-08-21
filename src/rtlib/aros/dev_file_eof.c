/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_eof.c

    Purpose:

        Detect end-of-file without desynchronizing POSIXC's buffered stream.

    Responsibilities:

        - compare logical stream positions for binary and random files
        - retain standard stream peeking for text and encoded input
        - serialize stream access with the runtime file lock

    This file intentionally does NOT contain:

        - generic Unix EOF behavior
        - file-size mutation
        - decoder logic
*/

#include "../fb.h"

int fb_DevFileEof( FB_FILE *handle )
{
	FILE *fp;
	fb_off_t position;
	int eof;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return FB_TRUE;
	}

	switch( handle->mode )
	{
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
		/* fb_FileEofEx() accounts for the runtime's put-back buffer. */
		position = fb_hArosGetFilePosition( fp );
		eof = (position < 0) ? feof( fp ) : (position >= handle->size);
		break;

	default:
		/* Text streams retain AROS's EOF character and newline behavior. */
		eof = feof( fp );
		if( !eof )
		{
			int character = getc( fp );

			eof = (character == EOF);
			if( !eof )
				ungetc( character, fp );
		}
		break;
	}

	FB_UNLOCK();
	return eof ? FB_TRUE : FB_FALSE;
}

/* end of aros/dev_file_eof.c */
