/* UTF-encoded file devices open */

#include "fb.h"

static FB_FILE_HOOKS hooks_dev_file = {
	fb_DevFileEof,
	fb_DevFileClose,
	fb_DevFileSeek,
	fb_DevFileTell,
	fb_DevFileReadEncod,
	fb_DevFileReadEncodWstr,
	fb_DevFileWriteEncod,
	fb_DevFileWriteEncodWstr,
	fb_DevFileLock,
	fb_DevFileUnlock,
	fb_DevFileReadLineEncod,
	fb_DevFileReadLineEncodWstr,
	NULL,
	fb_DevFileFlush
};

static int hCheckBOM( FILE *fp, FB_FILE_ENCOD encod )
{
	unsigned char bom[4];

	switch( encod )
	{
	case FB_FILE_ENCOD_UTF8:
		if( fread( bom, 1, 3, fp ) != 3 )
			return 0;

		return (bom[0] == 0xEF) && (bom[1] == 0xBB) && (bom[2] == 0xBF);

	case FB_FILE_ENCOD_UTF16:
		if( fread( bom, 1, sizeof( UTF_16 ), fp ) != sizeof( UTF_16 ) )
			return 0;

		return (bom[0] == 0xFF) && (bom[1] == 0xFE);

	case FB_FILE_ENCOD_UTF32:

		if( fread( bom, 1, sizeof( UTF_32 ), fp ) != sizeof( UTF_32 ) )
			return 0;

		return (bom[0] == 0xFF) && (bom[1] == 0xFE) &&
		       (bom[2] == 0x00) && (bom[3] == 0x00);

	default:
		return 0;
	}
}

static int hWriteBOM( FILE *fp, FB_FILE_ENCOD encod )
{
	switch( encod )
	{
	case FB_FILE_ENCOD_UTF8:
		{
			static const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
			if( fwrite( bom, 1, sizeof( bom ), fp ) != sizeof( bom ) )
				return 0;
		}
		break;

	case FB_FILE_ENCOD_UTF16:
		{
			static const unsigned char bom[] = { 0xFF, 0xFE };
			if( fwrite( bom, 1, sizeof( bom ), fp ) != sizeof( bom ) )
				return 0;
		}
		break;

	case FB_FILE_ENCOD_UTF32:
		{
			static const unsigned char bom[] = { 0xFF, 0xFE, 0x00, 0x00 };
			if( fwrite( bom, 1, sizeof( bom ), fp ) != sizeof( bom ) )
				return 0;
		}
		break;

	default:
		return 0;
	}

	return 1;
}

int fb_DevFileOpenEncod
	(
		FB_FILE *handle,
		const char *filename,
		size_t fname_len
	)
{
	FILE *fp = NULL;
	const char *openmask;
	char *fname;
	int effective_mode;

	FB_LOCK();

	fname = (char*) alloca(fname_len + 1);
	memcpy(fname, filename, fname_len);
	fname[fname_len] = 0;

	/* Convert directory separators to whatever the current platform supports */
	fb_hConvertPath( fname );

	handle->hooks = &hooks_dev_file;
	effective_mode = handle->mode;

	openmask = NULL;

	switch( handle->mode )
	{
	case FB_FILE_MODE_INPUT:
	case FB_FILE_MODE_APPEND:
		/*	Even in append mode, try and open for reading first 
			because trying to read the BOM in "ab" mode will fail 
		*/
		openmask = "rb";
		break;

	case FB_FILE_MODE_OUTPUT:
		openmask = "wb";
		break;

	default:
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	/* try opening */
	fp = fb_hOpenFile( fname, openmask );

	if( handle->mode == FB_FILE_MODE_APPEND )
	{
		/*	if we weren't able to open an existing file for
			append, then try writing instead 
		*/
		if( fp == NULL )
		{
			/* not found? handle mode as if output was specified */
			effective_mode = FB_FILE_MODE_OUTPUT;
			openmask = "ab";
			fp = fb_hOpenFile( fname, openmask );
		}
		else
		{
			fb_hSetFileBufSize( fp );

			if( !hCheckBOM( fp, handle->encod ) )
			{
				fclose( fp );
				FB_UNLOCK();
				return fb_ErrorSetNum( FB_RTERROR_FILEIO );
			}
			else
			{
				/* if we have the correct BOM, then reopen the file for append */
				openmask = "ab";
				fp = fb_hReopenFile( fname, openmask, fp );
			}
		}
	}

	/* not opened? */
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	}

	fb_hSetFileBufSize( fp );

	handle->opaque = fp;

	if ( handle->access == FB_FILE_ACCESS_ANY)
		handle->access = FB_FILE_ACCESS_READWRITE;

	switch( effective_mode )
	{
	case FB_FILE_MODE_INPUT:
		/* check the BOM if reading only */
		if( !hCheckBOM( fp, handle->encod ) )
		{
			fclose( fp );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		break;

	case FB_FILE_MODE_OUTPUT:
		/* write the BOM if file was just newly created */
		if( !hWriteBOM( fp, handle->encod ) )
		{
			fclose( fp );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
		}
		break;
	}

	/* calc file size */
	handle->size = fb_DevFileGetSize( fp, handle->mode, handle->encod, TRUE );
	if( handle->size == -1 )
	{
		fclose( fp );
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}
