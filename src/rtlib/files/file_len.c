/*
 * Project: FreeBASIC runtime library
 * File: files/file_len.c
 * Purpose: get file length by filename.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

fb_off_t fb_FileLenEx( const char *filename )
{
	FILE *fp;
	fb_off_t len;
	
	fp = fb_hOpenFile( filename, "rb" );	
	if( fp != NULL )
	{
		if( fseeko( fp, 0, SEEK_END ) == 0 ) 
		{
			if( (len = ftello( fp )) != -1 ) 
			{
				fclose( fp );
				fb_ErrorSetNum( FB_RTERROR_OK );
				return len;
			}
		}

		fclose( fp );
	}

	fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return 0;
}

FBCALL long long fb_FileLen( const char *filename )
{
	return fb_FileLenEx( filename );
}

/* end of files/file_len.c */
