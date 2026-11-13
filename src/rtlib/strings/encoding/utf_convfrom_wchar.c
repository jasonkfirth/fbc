/*
 * Project: FreeBASIC runtime library
 * File: strings/encoding/utf_convfrom_wchar.c
 * Purpose: wstring to UTF conversion (based on ConvertUTF.c free implementation from Unicode, Inc).
 * Responsibilities: Conversion between byte, wide and UTF encodings.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/* wstring to UTF conversion
 * (based on ConvertUTF.c free implementation from Unicode, Inc)
 */

#include "fb.h"
#include "utf_size.h"

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define FB_FALLTHROUGH __attribute__((fallthrough))
#else
#define FB_FALLTHROUGH ((void)0)
#endif

static void hUTF16ToUTF8( const FB_WCHAR *src, ssize_t chars, UTF_8 *dst, ssize_t *total_bytes )
{
	UTF_32 c;
	ssize_t bytes;

	*total_bytes = 0;
	while( chars > 0 )
	{
		c = *src++;
		if( c >= UTF16_SUR_HIGH_START && c <= UTF16_SUR_HIGH_END )
		{
			c = ((c - UTF16_SUR_HIGH_START) << UTF16_HALFSHIFT) +
			     (((UTF_32)*src++) - UTF16_SUR_LOW_START) + UTF16_HALFBASE;

			--chars;
		}

		if( c < (UTF_32)0x80 )
			bytes =	1;
		else if( c < (UTF_32)0x800 )
			bytes = 2;
		else if( c < (UTF_32)0x10000 )
			bytes = 3;
		else
			bytes =	4;

		dst += bytes;

		switch( bytes )
		{
		case 4:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 3:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 2:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 1:
			*--dst = (UTF_8)(c | fb_utf8_bmarkTb[bytes]);
		}

		dst += bytes;
		--chars;
		*total_bytes += bytes;
	}
}

static void hUTF32ToUTF8( const FB_WCHAR *src, ssize_t chars, UTF_8 *dst, ssize_t *total_bytes )
{
	UTF_32 c;
	ssize_t bytes;

	*total_bytes = 0;
	while( chars > 0 )
	{
		c = *src++;
		if( c < (UTF_32)0x80 )
			bytes =	1;
		else if( c < (UTF_32)0x800 )
			bytes = 2;
		else if( c < (UTF_32)0x10000 )
			bytes = 3;
		else
			bytes =	4;

		dst += bytes;

		switch( bytes )
		{
		case 4:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 3:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 2:
			*--dst = ((c | UTF8_BYTEMARK) & UTF8_BYTEMASK);
			c >>= 6;
			FB_FALLTHROUGH;
		case 1:
			*--dst = (UTF_8)(c | fb_utf8_bmarkTb[bytes]);
		}

		dst += bytes;
		--chars;
		*total_bytes += bytes;
	}
}

static char *hToUTF8( const FB_WCHAR *src, ssize_t chars, char *dst, ssize_t *bytes )
{
	size_t allocation;

	if( !fb_hUTFBytes( chars, 4, &allocation ) )
		return NULL;
	if( chars > 0 )
	{
		if( dst == NULL )
		{
			dst = malloc( allocation );
			if( dst == NULL )
				return NULL;
		}
	}

	switch( sizeof( FB_WCHAR ) )
	{
	case sizeof( UTF_8 ):
        fb_hCharToUTF8( (const char *)src, chars, dst, bytes );
		break;

	case sizeof( UTF_16 ):
		hUTF16ToUTF8( src, chars, (UTF_8 *)dst, bytes );
		break;

	case sizeof( UTF_32 ):
        hUTF32ToUTF8( src, chars, (UTF_8 *)dst, bytes );
		break;
	}

	return dst;
}

static void hCharToUTF16( const FB_WCHAR *src, ssize_t chars, UTF_16 *dst )
{
	while( chars > 0 )
	{
		*dst++ = (unsigned char)*src++;
		--chars;
	}
}

/* The caller either owns a sufficiently large output buffer or has
   allocated the checked worst-case size. Never realloc caller-owned storage. */
static void hUTF32ToUTF16( const FB_WCHAR *src, ssize_t chars, UTF_16 *dst, ssize_t *bytes )
{
	UTF_32 c;

	while( chars > 0 )
	{
		c = (UTF_32)*src++;
		if( c > UTF16_MAX_BMP )
		{
			c -= UTF16_HALFBASE;
			*dst++ = (UTF_16)((c >> UTF16_HALFSHIFT) + UTF16_SUR_HIGH_START);
			c = ((c & UTF16_HALFMASK) + UTF16_SUR_LOW_START);
			*bytes += (ssize_t)sizeof( UTF_16 );
		}
		*dst++ = (UTF_16)c;
		--chars;
	}
}

static char *hToUTF16( const FB_WCHAR *src, ssize_t chars, char *dst, ssize_t *bytes )
{
	/* UTF16/UTF32 conversion APIs expose native typed code units.
	   Encoded file output normalizes these units to little endian. */

	size_t allocation;

	if( !fb_hUTFBytes( chars, ((sizeof( FB_WCHAR ) == sizeof( UTF_32 )) ? 4 : 2), &allocation ) )
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

	switch( sizeof( FB_WCHAR ) )
	{
	case sizeof( char ):
		hCharToUTF16( src, chars, (UTF_16 *)dst );
		break;

	case sizeof( UTF_16 ):
		memcpy( dst, src, (size_t)*bytes );
		break;

	case sizeof( UTF_32 ):
        hUTF32ToUTF16( src, chars, (UTF_16 *)dst, bytes );
		break;
	}

	return dst;
}

static void hCharToUTF32( const FB_WCHAR *src, ssize_t chars, UTF_32 *dst )
{
	while( chars > 0 )
	{
		*dst++ = (unsigned char)*src++;
		--chars;
	}
}

static void hUTF16ToUTF32( const FB_WCHAR *src, ssize_t chars, UTF_32 *dst, ssize_t *bytes )
{
	UTF_32 c;

	while( chars > 0 )
	{
		c = (UTF_32)*src++;
		if( c >= UTF16_SUR_HIGH_START && c <= UTF16_SUR_HIGH_END )
		{
			c = ((c - UTF16_SUR_HIGH_START) << UTF16_HALFSHIFT) +
			     (((UTF_32)*src++) - UTF16_SUR_LOW_START) + UTF16_HALFBASE;

			*bytes -= (ssize_t)sizeof( UTF_32 );
			--chars;
		}

		*dst++ = c;

		--chars;
	}
}

static char *hToUTF32( const FB_WCHAR *src, ssize_t chars, char *dst, ssize_t *bytes )
{
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

	switch( sizeof( FB_WCHAR ) )
	{
	case sizeof( char ):
		hCharToUTF32( src, chars, (UTF_32 *)dst );
		break;

	case sizeof( UTF_16 ):
        hUTF16ToUTF32( src, chars, (UTF_32 *)dst, bytes );
		break;

	case sizeof( UTF_32 ):
		memcpy( dst, src, (size_t)*bytes );
		break;

	default:
		break;
	}

	return dst;
}

/* chars is the length of the input in characters, with UTF16 surrogate pairs
   counting as 2.
   dst is an optional output buffer (which must be large enough); if NULL, one
   is malloc'd.
   A NUL is only appended to the output if it occurs in the input (unlike
   fb_UTFToWChar this does NOT stop on seeing a NUL, as the length is known).
   Returns the output buffer and sets *bytes to the number of bytes written.
*/
char *fb_WCharToUTF
	(
		FB_FILE_ENCOD encod,
		const FB_WCHAR *src,
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

/* end of strings/encoding/utf_convfrom_wchar.c */
