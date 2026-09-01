/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_size.c

    Purpose:

        Query AROS file sizes without a seek-to-end sequence while keeping
        ordinary positions inside the buffered FILE abstraction.

    Responsibilities:

        - obtain an AROS DOS handle from a POSIXC FILE stream
        - query file metadata through dos.library's stable interface
        - seek and tell through POSIXC so its buffered state remains coherent
        - position encoded input immediately after its byte-order mark
        - maintain a logical size after successful writes and extensions

    This file intentionally does NOT contain:

        - generic Unix file-size behavior
        - AROS architecture policy
        - file decoder logic

    AROS POSIXC behavior:

        POSIXC's fseeko-to-end size calculation can leave an MT stream at
        end-of-file while FreeBASIC's recursive runtime lock is held.  The
        dos.library metadata API does not change either the native or buffered
        position.  AROS's dos64.library currently misreports the size of some
        FAT files as their current position, so its direct API and POSIXC's
        new 64-bit seek engine are unsuitable here.  Flush() reconciles the
        DOS and stdio buffers before the stable dos.library metadata and
        positioning calls are used.
*/

#include "../fb.h"

#include <dos/dos.h>
#include <proto/dos.h>

#include <sys/stat.h>

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

	#if defined(__arm__)
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
	#else
	return fb_hArosSetFilePosition( fp, offset, SEEK_SET );
	#endif
}

fb_off_t fb_DevFileGetSize( FILE *fp, int mode, FB_FILE_ENCOD encod,
			    int seek_back )
{
	#if !defined(__arm__)
	BPTR file_handle;
	#else
	struct stat status;
	#endif
	fb_off_t size;

	#if !defined(__arm__)
	if( fb_hArosGetFileHandle( fp, &file_handle ) != 0 )
		return -1;
	#endif

	switch( mode )
	{
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
	case FB_FILE_MODE_INPUT:
	#if defined(__arm__)
		if( fstat( fileno( fp ), &status ) != 0 )
			return -1;

		size = (fb_off_t)status.st_size;
	#else
		size = fb_hArosGetFileSize( file_handle );
		if( size < 0 )
			return -1;
	#endif

		if( seek_back && fb_hDevFileSeekStart( fp, mode, encod, TRUE ) != 0 )
			return -1;

		return size;

	case FB_FILE_MODE_APPEND:
	case FB_FILE_MODE_OUTPUT:
	#if defined(__arm__)
		if( mode == FB_FILE_MODE_OUTPUT )
			return 0;

		if( fstat( fileno( fp ), &status ) != 0 )
			return -1;
		return (fb_off_t)status.st_size;
	#else
		return fb_hArosGetFilePosition( fp );
	#endif

	default:
		return 0;
	}
}

/* end of aros/dev_file_size.c */
