/*
 * Project: FreeBASIC runtime library
 * File: strings/encoding/utf_convto_char.c
 * Purpose: UTF to zstring conversion (based on ConvertUTF.c free implementation from Unicode, Inc).
 * Responsibilities: Conversion between byte, wide and UTF encodings.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/* UTF to zstring conversion
 * (based on ConvertUTF.c free implementation from Unicode, Inc)
 */

#include "fb.h"
#include "utf_size.h"

/* Use the shared declarations so readers keep the same ABI labels. */

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define FB_FALLTHROUGH __attribute__((fallthrough))
#else
#define FB_FALLTHROUGH ((void)0)
#endif


#define FB_UTF8_MAX_TRAILING_BYTES 5

/*
char * fb_hUTF8ToChar( const UTF_8 *src, char *dst, ssize_t *chars )
char * fb_hUTF16ToChar( const UTF_8 *src, char *dst, ssize_t *chars )
char * fb_hUTF32ToChar( const UTF_8 *src, char *dst, ssize_t *chars )

if 'dst' is null

	src 
		- 'src' is the address of the source utf-8 encoded string
		- must not be null
		- must have a null terminating character

	dst
		- ignored

	chars
		- 'chars' must not be null
		- 'chars' value is ignored on entry
		- on return, 'chars' is set to the number of characters 
		  converted not including the null terminator

	return value
		- the pointer to the newly allocated (realloc) memory
		  containing the converted string which includes a
		  terminating null character, or NULL pointer if
		  unable to allocate memory
		- 'chars' contains the number of characters 
		  converted not including the null terminator

	NOTE: caller is responsible to free the memory


if 'dst' is not null

	src
		- 'src' is the address of the source utf-8 encoded string
		- must not be null
		- may have null terminating character

	dst
		- 'dst' is the destination buffer for the ascii string

	chars
		- 'chars' must not be null
		- caller must set 'chars' to the maximum number of
		  characters to convert which may include the 
		  terminating null character
		- on return, 'chars' is set to the number characters
		  converted not including the terminating null
		  character
	
	NOTE: the caller will not know if a terminating null
	character was written based on the value of 'chars'.  The 
	value of 'chars' will be the same if the conversion ended
	on a terminating null character or the character
	immediately before it.  In either case, the terminating
	null character is not written.

*/

char *fb_hUTF8ToChar( const UTF_8 *src, char *dst, ssize_t *chars )
{
	UTF_32 c;
	ssize_t charsleft;
	unsigned int extbytes;
	char *buffer = dst;
	
    if( dst == NULL )
    {
		ssize_t dst_size = 0;
	    charsleft = 0;
	    do 
	    {
			extbytes = fb_utf8_trailingTb[(unsigned int)*src];
	
			c = 0;
			if( extbytes > FB_UTF8_MAX_TRAILING_BYTES )
			{
				++src;
				c = '?';
			}
			else switch( extbytes )
			{
				case 5:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 4:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 3:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 2:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 1:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 0:
					c += *src++;
			}

			if( extbytes <= FB_UTF8_MAX_TRAILING_BYTES )
				c -= fb_utf8_offsetsTb[extbytes];
	
			if( c > 255 )
				c = '?';
	
			if( charsleft == 0 )
			{
				size_t allocation;
				if( !fb_hUTFGrow( &dst_size, charsleft, sizeof( char ), &allocation ) ) {
					free( buffer );
					return NULL;
				}
				charsleft = 8;
				char *newbuffer = realloc( buffer, allocation );
				if( newbuffer == NULL )
				{
					free( buffer );
					return NULL;
				}
				buffer = newbuffer;
				dst = buffer + dst_size - charsleft;
			}
			
			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		} while( 1 );
		
		*chars = dst_size - charsleft;
	}
	else
	{
	    charsleft = *chars;
	    while( charsleft > 0 )
	    {
			extbytes = fb_utf8_trailingTb[*src];
	
			c = 0;
			if( extbytes > FB_UTF8_MAX_TRAILING_BYTES )
			{
				++src;
				c = '?';
			}
			else switch( extbytes )
			{
				case 5:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 4:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 3:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 2:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 1:
					c += *src++; c <<= 6;
					FB_FALLTHROUGH;
				case 0:
					c += *src++;
			}

			if( extbytes <= FB_UTF8_MAX_TRAILING_BYTES )
				c -= fb_utf8_offsetsTb[extbytes];

			if( c > 255 )
				c = '?';

			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		}
		
		*chars -= charsleft;
	}
	
	return buffer;
}

char *fb_hUTF16ToChar( const UTF_16 *src, char *dst, ssize_t *chars )
{
	UTF_16 c;
	ssize_t charsleft;
	char *buffer = dst;

    if( dst == NULL )
    {
		ssize_t dst_size = 0;
	    charsleft = 0;
	    do 
	    {
	    	c = *src++;
			if( c > 255 )
			{
				if( c >= UTF16_SUR_HIGH_START && c <= UTF16_SUR_HIGH_END )
	    			++src;
	    		c = '?';
	    	}
	
			if( charsleft == 0 )
			{
				size_t allocation;
				if( !fb_hUTFGrow( &dst_size, charsleft, sizeof( char ), &allocation ) ) {
					free( buffer );
					return NULL;
				}
				charsleft = 8;
				char *newbuffer = realloc( buffer, allocation );
				if( newbuffer == NULL )
				{
					free( buffer );
					return NULL;
				}
				buffer = newbuffer;
				dst = buffer + dst_size - charsleft;
			}
			
			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		} while( 1 );
		
		*chars = dst_size - charsleft;
	}
	else
	{
	    charsleft = *chars;
	    while( charsleft > 0 )
	    {
	    	c = *src++;
			if( c > 255 )
			{
				if( c >= UTF16_SUR_HIGH_START && c <= UTF16_SUR_HIGH_END )
	    			++src;
				c = '?';
			}

			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		}
		
		*chars -= charsleft;
	}
	
	return buffer;
}

char *fb_hUTF32ToChar( const UTF_32 *src, char *dst, ssize_t *chars )
{
	UTF_32 c;
	ssize_t charsleft;
	char *buffer = dst;

    if( dst == NULL )
    {
		ssize_t dst_size = 0;
	    charsleft = 0;
	    do 
	    {
	    	c = *src++;
			if( c > 255 )
				c = '?';
	
			if( charsleft == 0 )
			{
				size_t allocation;
				if( !fb_hUTFGrow( &dst_size, charsleft, sizeof( char ), &allocation ) ) {
					free( buffer );
					return NULL;
				}
				charsleft = 8;
				char *newbuffer = realloc( buffer, allocation );
				if( newbuffer == NULL )
				{
					free( buffer );
					return NULL;
				}
				buffer = newbuffer;
				dst = buffer + dst_size - charsleft;
			}
			
			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		} while( 1 );
		
		*chars = dst_size - charsleft;
	}
	else
	{
	    charsleft = *chars;
	    while( charsleft > 0 )
	    {
	    	c = *src++;
			if( c > 255 )
				c = '?';

			*dst++ = (char)c;

			if( c == 0 )
				break;
			
			--charsleft;
		}
		
		*chars -= charsleft;
	}
	
	return buffer;
}

char *fb_UTFToChar( FB_FILE_ENCOD encod, const void *src, char *dst, ssize_t *chars )
{
	if( chars == NULL )
		return NULL;
	if( src == NULL || (dst != NULL && *chars < 0) ) {
		*chars = 0;
		return NULL;
	}
	switch( encod )
	{
	case FB_FILE_ENCOD_UTF8:
		return fb_hUTF8ToChar( src, dst, chars );

	case FB_FILE_ENCOD_UTF16:
		return fb_hUTF16ToChar( src, dst, chars );

	case FB_FILE_ENCOD_UTF32:
		return fb_hUTF32ToChar( src, dst, chars );

	default:
		return NULL;
	}

}

/* end of strings/encoding/utf_convto_char.c */
