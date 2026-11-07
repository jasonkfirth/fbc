/*
 * Project: FreeBASIC runtime library
 * File: strings/encoding/utf_convfrom_char.c
 * Purpose: ascii to UTF conversion.
 * Responsibilities: Conversion between byte, wide and UTF encodings.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include "utf_size.h"

static char *hToUTF8( const char *src, ssize_t chars, char *dst, ssize_t *bytes )
{
	size_t allocation;

	if( !fb_hUTFBytes( chars, 2, &allocation ) )
		return NULL;
	if( chars > 0 )
	{
		if( dst == NULL )
		{
			dst = malloc( allocation );
			if( dst == NULL )
				return NULL;
		}

		fb_hCharToUTF8( src, chars, dst, bytes );
	}
	else
		*bytes = 0;

	return dst;
}

static char *hToUTF16( const char *src, ssize_t chars, char *dst, ssize_t *bytes )
{
	UTF_16 *p;

	/* UTF16/UTF32 conversion APIs expose native typed code units.
	   Encoded file output normalizes these units to little endian. */

	size_t allocation;

	if( !fb_hUTFBytes( chars, sizeof( UTF_16 ), &allocation ) )
		return NULL;
	*bytes = chars * (ssize_t)sizeof( UTF_16 );

	if( chars > 0 )
	{
		if( dst == NULL )
		{
			dst = malloc( allocation );
			if( dst == NULL )
				return NULL;
		}
	}

	p = (UTF_16 *)dst;
	while( chars > 0 )
	{
		*p++ = (unsigned char)*src++;
		--chars;
	}

	return dst;
}

static char *hToUTF32( const char *src, ssize_t chars, char *dst, ssize_t *bytes )
{
	UTF_32 *p;

	/* UTF16/UTF32 conversion APIs expose native typed code units.
	   Encoded file output normalizes these units to little endian. */

	size_t allocation;

	if( !fb_hUTFBytes( chars, sizeof( UTF_32 ), &allocation ) )
		return NULL;
	*bytes = chars * (ssize_t)sizeof( UTF_32 );

	if( chars > 0 )
	{
		if( dst == NULL )
		{
			dst = malloc( allocation );
			if( dst == NULL )
				return NULL;
		}
	}

	p = (UTF_32 *)dst;
	while( chars > 0 )
	{
		*p++ = (unsigned char)*src++;
		--chars;
	}

	return dst;
}

char *fb_CharToUTF
	(
		FB_FILE_ENCOD encod,
		const char *src,
		ssize_t chars,
		char *dst,
		ssize_t *bytes
	)
{
	if( bytes == NULL )
		return NULL;
	*bytes = 0;
	if( chars < 0 || (chars > 0 && src == NULL) )
		return NULL;
	switch( encod )
	{
	case FB_FILE_ENCOD_UTF8:
		return hToUTF8( src, chars, dst, bytes );

	case FB_FILE_ENCOD_UTF16:
		return hToUTF16( src, chars, dst, bytes );

	case FB_FILE_ENCOD_UTF32:
		return hToUTF32( src, chars, dst, bytes );

	default:
		return NULL;
	}
}

/* end of strings/encoding/utf_convfrom_char.c */
