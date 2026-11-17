/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_read_wstr.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileReadWstr( FB_FILE *handle, FB_WCHAR *dst, size_t *pchars )
{
    FILE *fp;
	size_t chars, read_chars;
	ssize_t converted_chars;
    char *buffer;
    FB_WCHAR *wbuffer;
    int heap_allocated;
    char local_buffer[FB_LOCALBUFF_MAXLEN];
    FB_WCHAR local_wbuffer[FB_LOCALBUFF_MAXLEN];

    FB_LOCK();

    if( handle == NULL )
        fp = stdin;
    else
    {
        fp = (FILE*) handle->opaque;
        if( fp == stdout || fp == stderr )
            fp = stdin;

        if( fp == NULL )
        {
            FB_UNLOCK();
            return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        }
    }

	chars = *pchars;
	heap_allocated = FALSE;

	if( (chars > ((size_t)PTRDIFF_MAX / sizeof( FB_WCHAR ) - 1)) ||
	    (chars > (((size_t)-1) - 1)) ||
	    (chars > ((((size_t)-1) / sizeof( FB_WCHAR )) - 1)) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	if( chars < FB_LOCALBUFF_MAXLEN )
	{
		buffer = local_buffer;
		wbuffer = local_wbuffer;
	}
	else
	{
		buffer = (char *)malloc( chars + 1 );
		if( buffer == NULL )
		{
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

		wbuffer = (FB_WCHAR *)malloc( (chars + 1) * sizeof( FB_WCHAR ) );
		if( wbuffer == NULL )
		{
			free( buffer );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

		heap_allocated = TRUE;
	}

	memset( buffer, 0, chars + 1 );

	/* do read */
	read_chars = fread( buffer, 1, chars, fp );
	if( read_chars > chars )
		read_chars = chars;

	/* convert to wchar, file should be opened with the ENCODING option
	   to allow UTF characters to be read */
	converted_chars = fb_wstr_ConvFromA( wbuffer, (ssize_t)read_chars, buffer );
	if( converted_chars < 0 )
		converted_chars = 0;
	if( converted_chars > (ssize_t)chars )
		converted_chars = (ssize_t)chars;

	memcpy( dst, wbuffer, (size_t)converted_chars * sizeof( FB_WCHAR ) );

	if( heap_allocated )
	{
		free( buffer );
		free( wbuffer );
	}

	/* fill with nulls if at eof */
	if( (size_t)converted_chars != *pchars )
		memset( (void *)&dst[converted_chars], 0,
		        (*pchars - (size_t)converted_chars) * sizeof( FB_WCHAR ) );

	*pchars = (size_t)converted_chars;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_read_wstr.c */
