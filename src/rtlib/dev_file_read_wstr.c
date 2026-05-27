/* file device */

#include "fb.h"

int fb_DevFileReadWstr( FB_FILE *handle, FB_WCHAR *dst, size_t *pchars )
{
    FILE *fp;
    size_t chars, read_chars, converted_chars;
    char *buffer;
    FB_WCHAR *wbuffer;

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

	if( (chars > (((size_t)-1) - 1)) ||
	    (chars > ((((size_t)-1) / sizeof( FB_WCHAR )) - 1)) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	if( chars < FB_LOCALBUFF_MAXLEN )
	{
		buffer = alloca( chars + 1 );
		wbuffer = alloca( (chars + 1) * sizeof( FB_WCHAR ) );
		/* note: if out of memory on alloca, it's a stack exception */
	}
	else
	{
		buffer = malloc( chars + 1 );
		if( buffer == NULL )
		{
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}

		wbuffer = malloc( (chars + 1) * sizeof( FB_WCHAR ) );
		if( wbuffer == NULL )
		{
			free( buffer );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}
	}

	memset( buffer, 0, chars + 1 );

	/* do read */
	read_chars = fread( buffer, 1, chars, fp );
	if( read_chars > chars )
		read_chars = chars;

	/* convert to wchar, file should be opened with the ENCODING option
	   to allow UTF characters to be read */
	converted_chars = fb_wstr_ConvFromA( wbuffer, read_chars, buffer );
	if( converted_chars > chars )
		converted_chars = chars;

	memcpy( dst, wbuffer, converted_chars * sizeof( FB_WCHAR ) );

	if( *pchars >= FB_LOCALBUFF_MAXLEN )
	{
		free( buffer );
		free( wbuffer );
	}

	/* fill with nulls if at eof */
	if( converted_chars != *pchars )
        memset( (void *)&dst[converted_chars], 0, (*pchars - converted_chars) * sizeof( FB_WCHAR ) );

    *pchars = converted_chars;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}
