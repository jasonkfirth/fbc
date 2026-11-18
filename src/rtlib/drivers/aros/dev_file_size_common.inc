/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_file_size_common.inc

    Purpose:

        Share the native DOS handle and position helpers used by AROS file
        size implementations.

    Responsibilities:

        - resolve POSIXC streams to native DOS file handles
        - query file metadata through dos.library
        - keep DOS and stdio positions synchronized
        - extend files when a native seek moves beyond end-of-file

    This include intentionally does NOT contain:

        - ARM or non-ARM file-size policy
        - file-device dispatch or decoding
*/

#ifndef FB_RTLIB_AROS_DEV_FILE_SIZE_COMMON_INC
#define FB_RTLIB_AROS_DEV_FILE_SIZE_COMMON_INC

#include "../../fb.h"
#include <dos/dos.h>
#include <proto/dos.h>

/*
 * Resolve the native handle instead of depending on FILE's implementation
 * layout.  __get_default_file() is the public POSIXC compatibility bridge
 * used by AROS itself for this purpose.
 */
int fb_hArosGetFileHandle( FILE *fp, BPTR *file_handle )
{
	int descriptor;

	if( fp == NULL || file_handle == NULL )
		return -1;

	descriptor = fileno( fp );
	if( descriptor < 0 )
		return -1;

	return __get_default_file( descriptor, (long *)file_handle );
}

static fb_off_t fb_hArosGetFileSize( BPTR file_handle )
{
	struct FileInfoBlock *file_info;
	fb_off_t size = -1;

	file_info = AllocDosObject( DOS_FIB, NULL );
	if( file_info != NULL )
	{
		if( ExamineFH( file_handle, file_info ) )
		{
			/* Preserve the full unsigned range of the traditional FIB. */
			size = (fb_off_t)(ULONG)file_info->fib_Size;
		}

		FreeDosObject( DOS_FIB, file_info );
	}

	return size;
}

fb_off_t fb_hArosGetFilePosition( FILE *fp )
{
	BPTR file_handle;
	LONG position;

	if( fb_hArosGetFileHandle( fp, &file_handle ) != 0 )
		return -1;

	Flush( file_handle );
	position = Seek( file_handle, 0, OFFSET_CURRENT );
	return (position == -1) ? -1 : (fb_off_t)position;
}

void fb_hArosGrowFileSize( FB_FILE *handle )
{
	fb_off_t position;

	if( handle == NULL || handle->opaque == NULL )
		return;

	position = fb_hArosGetFilePosition( (FILE *)handle->opaque );
	if( position > handle->size )
		handle->size = position;
}

int fb_hArosSetFilePosition( FILE *fp, fb_off_t position, int whence )
{
	BPTR file_handle;
	fb_off_t base_position;
	fb_off_t file_size;
	fb_off_t target_position;
	LONG original_position;
	LONG remaining;
	unsigned char zeroes[4096] = { 0 };

	if( fb_hArosGetFileHandle( fp, &file_handle ) != 0 )
		return -1;

	Flush( file_handle );
	original_position = Seek( file_handle, 0, OFFSET_CURRENT );
	if( original_position == -1 )
		return -1;

	file_size = fb_hArosGetFileSize( file_handle );
	if( file_size < 0 )
		return -1;

	switch( whence )
	{
	case SEEK_SET:
		base_position = 0;
		break;

	case SEEK_CUR:
		base_position = original_position;
		break;

	case SEEK_END:
		base_position = file_size;
		break;

	default:
		return -1;
	}

	/* Native dos.library positions are signed 32-bit on these AROS targets. */
	if( position < -base_position || position > INT32_MAX - base_position )
		return -1;
	target_position = base_position + position;

	if( target_position <= file_size )
	{
		if( Seek( file_handle, (LONG)target_position,
			  OFFSET_BEGINNING ) == -1 )
			return -1;
	}
	else
	{
		/*
		 * AROS handlers reject seeks beyond EOF.  Reproduce POSIX stream
		 * semantics by extending writable files with zeroes to the desired
		 * position.  A failed write restores both size and position.
		 */
		if( Seek( file_handle, 0, OFFSET_END ) == -1 )
			return -1;

		remaining = (LONG)(target_position - file_size);
		while( remaining > 0 )
		{
			LONG chunk = MIN( remaining, (LONG)sizeof( zeroes ) );

			if( Write( file_handle, zeroes, chunk ) != chunk )
			{
				SetFileSize( file_handle, (LONG)file_size,
					     OFFSET_BEGINNING );
				Seek( file_handle, original_position,
				      OFFSET_BEGINNING );
				return -1;
			}

			remaining -= chunk;
		}
		Flush( file_handle );
	}

	clearerr( fp );
	return 0;
}


#endif

/* end of drivers/aros/dev_file_size_common.inc */
