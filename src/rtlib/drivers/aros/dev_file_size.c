/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_file_size.c

    Purpose:

        Query file sizes and positions on non-ARM AROS targets.

    Responsibilities:

        - use the shared dos.library file helpers
        - preserve encoded input positions and logical file sizes

    This file intentionally does NOT contain:

        - ARM POSIXC stream handling, which is in arm/dev_file_size.c
        - file decoding or generic file-device policy
*/

#include "dev_file_size_common.inc"

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

	return fb_hArosSetFilePosition( fp, offset, SEEK_SET );
}

fb_off_t fb_DevFileGetSize( FILE *fp, int mode, FB_FILE_ENCOD encod,
			    int seek_back )
{
	BPTR file_handle;
	fb_off_t size;

	if( fb_hArosGetFileHandle( fp, &file_handle ) != 0 )
		return -1;

	switch( mode )
	{
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
	case FB_FILE_MODE_INPUT:
		size = fb_hArosGetFileSize( file_handle );
		if( size < 0 )
			return -1;

		if( seek_back && fb_hDevFileSeekStart( fp, mode, encod, TRUE ) != 0 )
			return -1;

		return size;

	case FB_FILE_MODE_APPEND:
	case FB_FILE_MODE_OUTPUT:
		return fb_hArosGetFilePosition( fp );

	default:
		return 0;
	}
}

/* end of drivers/aros/dev_file_size.c */
