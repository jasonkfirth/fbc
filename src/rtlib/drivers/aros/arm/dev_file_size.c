/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/arm/dev_file_size.c

    Purpose:

        Query file sizes on AROS ARM while keeping POSIXC stream state valid.

    Responsibilities:

        - use fstat() for the ARM file-size path
        - avoid affected FAT handler seeks when consuming encoded input marks
        - maintain logical file sizes after writes and extensions

    This file intentionally does NOT contain:

        - shared AROS DOS helpers, which are in ../dev_file_size_common.inc
        - file decoding or generic file-device policy
*/

#include "../dev_file_size_common.inc"
#include <sys/stat.h>

int fb_hDevFileSeekStart( FILE *fp, int mode, FB_FILE_ENCOD encod,
			 int seek_zero )
{
	fb_off_t offset;

	(void)mode;

	switch( encod )
	{
	case FB_FILE_ENCOD_UTF8:
		offset = 3;
		break;

	case FB_FILE_ENCOD_UTF16:
		offset = sizeof( UTF_16 );
		break;

	case FB_FILE_ENCOD_UTF32:
		offset = sizeof( UTF_32 );
		break;

	default:
		if( seek_zero == FALSE )
			return 0;

		offset = 0;
	}

	/*
	 * ARM's FAT handler can receive a corrupt port when either the native
	 * handle bridge or POSIXC issues Seek() on a freshly opened stream.  File
	 * open calls this routine while the stream is still at byte zero, so the
	 * ordinary case needs no positioning operation.  Encoded input advances
	 * over its short byte-order mark without asking the handler to seek.
	 */
	if( offset == 0 )
		return 0;

	while( offset > 0 )
	{
		if( fgetc( fp ) == EOF )
			return -1;
		--offset;
	}
	return 0;
}

fb_off_t fb_DevFileGetSize( FILE *fp, int mode, FB_FILE_ENCOD encod,
			    int seek_back )
{
	struct stat status;
	fb_off_t size;

	switch( mode )
	{
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
	case FB_FILE_MODE_INPUT:
		if( fstat( fileno( fp ), &status ) != 0 )
			return -1;

		size = (fb_off_t)status.st_size;

		if( seek_back && fb_hDevFileSeekStart( fp, mode, encod, TRUE ) != 0 )
			return -1;

		return size;

	case FB_FILE_MODE_APPEND:
	case FB_FILE_MODE_OUTPUT:
		if( mode == FB_FILE_MODE_OUTPUT )
			return 0;

		if( fstat( fileno( fp ), &status ) != 0 )
			return -1;
		return (fb_off_t)status.st_size;

	default:
		return 0;
	}
}

/* end of drivers/aros/arm/dev_file_size.c */
