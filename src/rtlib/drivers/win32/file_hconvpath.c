/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/file_hconvpath.c

    Purpose:

        Adapt FreeBASIC path strings to the host filesystem interfaces.

    Responsibilities:

        - normalize directory separators
        - convert UTF-8 or system-codepage paths for Windows Unicode APIs
        - map Win9x UTF-8 path components to reversible ANSI aliases
        - select filesystem calls supported by each desktop Windows family

    This file intentionally does NOT contain: - directory enumeration
        - file-handle bookkeeping
        - path searching or canonicalization
*/

#include "../../fb.h"
#include <errno.h>
#include <stdint.h>

#ifdef HOST_MINGW
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/* ------------------------------------------------------------------------- */
/* Path separator conversion                                                 */
/* ------------------------------------------------------------------------- */

void fb_hConvertPath( char *path )
{
	size_t i, len;

	DBG_ASSERT( path != NULL );

	len = strlen( path );
	for( i = 0; i < len; i++ )
	{
#if defined( HOST_MINGW )
		if( path[i] == '/' )
			path[i] = '\\';
#else
		if( path[i] == '\\' )
			path[i] = '/';
#endif
	}
}

#ifdef HOST_MINGW
/* ------------------------------------------------------------------------- */
/* Desktop Windows path conversion                                           */
/* ------------------------------------------------------------------------- */

#define FB_WIN9X_PATH_PREFIX "_FBCU_"
#define FB_WIN9X_PATH_PREFIX_LENGTH (sizeof(FB_WIN9X_PATH_PREFIX) - 1u)
/* FAT long-file-name components are limited to 255 UTF-16 code units. */
#define FB_WIN9X_MAX_COMPONENT_LENGTH 255u

/* ------------------------------------------------------------------------- */
/* UTF-8 validation and Win9x long-name aliases                              */
/* ------------------------------------------------------------------------- */

static int fb_hIsValidUtf8( const unsigned char *text, size_t length,
                            int *contains_non_ascii )
{
	size_t index = 0;

	*contains_non_ascii = FALSE;
	while( index < length ) {
		unsigned char first = text[index];
		size_t continuation_count;

		if( first < 0x80u ) {
			index++;
			continue;
		}

		*contains_non_ascii = TRUE;
		if( first >= 0xc2u && first <= 0xdfu )
			continuation_count = 1u;
		else if( first >= 0xe0u && first <= 0xefu )
			continuation_count = 2u;
		else if( first >= 0xf0u && first <= 0xf4u )
			continuation_count = 3u;
		else
			return FALSE;

		if( continuation_count >= length - index )
			return FALSE;

		for( size_t continuation = 1u;
		     continuation <= continuation_count; ++continuation ) {
			unsigned char current = text[index + continuation];
			if( current < 0x80u || current > 0xbfu )
				return FALSE;
		}

		if( (first == 0xe0u && text[index + 1u] < 0xa0u) ||
		    (first == 0xedu && text[index + 1u] > 0x9fu) ||
		    (first == 0xf0u && text[index + 1u] < 0x90u) ||
		    (first == 0xf4u && text[index + 1u] > 0x8fu) )
			return FALSE;

		index += continuation_count + 1u;
	}

	return TRUE;
}

static int fb_hStartsWithWin9xPathPrefix( const unsigned char *text,
	                                       size_t length )
{
	static const unsigned char prefix[] = FB_WIN9X_PATH_PREFIX;
	size_t index;

	if( length < FB_WIN9X_PATH_PREFIX_LENGTH )
		return FALSE;

	for( index = 0; index < FB_WIN9X_PATH_PREFIX_LENGTH; ++index ) {
		unsigned char current = text[index];
		unsigned char expected = prefix[index];
		if( current >= 'a' && current <= 'z' )
			current = (unsigned char)(current - ('a' - 'A'));
		if( current != expected )
			return FALSE;
	}

	return TRUE;
}

static int fb_hHexValue( unsigned char character )
{
	if( character >= '0' && character <= '9' )
		return character - '0';
	if( character >= 'A' && character <= 'F' )
		return character - 'A' + 10;
	if( character >= 'a' && character <= 'f' )
		return character - 'a' + 10;
	return -1;
}

/*
   Win9x exposes filesystem paths through ANSI APIs. Valid UTF-8 components
   are stored under reversible ASCII aliases so FreeBASIC path operations
   retain the same logical name without depending on the active code page.
   The runtime decodes those aliases when Dir() enumerates the directory.
*/
char *fb_hConvertPathToAC( const char *path )
{
	char *normalized;
	char *result;
	size_t path_length;
	size_t capacity;
	size_t input_position = 0;
	size_t output_position = 0;
	size_t unc_components = 0;
	int unc_path;

	DBG_ASSERT( path != NULL );
	if( path == NULL ) {
		errno = EINVAL;
		return NULL;
	}

	path_length = strlen( path );
	if( path_length > SIZE_MAX - 1u || path_length > (SIZE_MAX - 1u) / 8u ) {
		errno = ENAMETOOLONG;
		return NULL;
	}
	capacity = path_length * 8u + 1u;
	normalized = (char*)malloc( path_length + 1u );
	if( normalized == NULL )
		return NULL;
	memcpy( normalized, path, path_length + 1u );

	/* The extended prefix is an NT path convention, not a Win9x one. */
	if( strncmp( normalized, "\\\\?\\UNC\\", 8 ) == 0 ) {
		size_t remaining = strlen( normalized + 8 ) + 1u;
		memmove( normalized + 2, normalized + 8, remaining );
		normalized[0] = '\\';
		normalized[1] = '\\';
	} else if( strncmp( normalized, "\\\\?\\", 4 ) == 0 ) {
		size_t remaining = strlen( normalized + 4 ) + 1u;
		memmove( normalized, normalized + 4, remaining );
	}

	fb_hConvertPath( normalized );
	unc_path = (path_length >= 2u && normalized[0] == '\\' &&
	             normalized[1] == '\\');
	result = (char*)malloc( capacity );
	if( result == NULL ) {
		free( normalized );
		return NULL;
	}

	while( normalized[input_position] != '\0' ) {
		size_t component_start;
		size_t component_length;
		int encode_component = FALSE;
		int contains_non_ascii = FALSE;
		int has_wildcard = FALSE;

		if( normalized[input_position] == '\\' ) {
			result[output_position++] = normalized[input_position++];
			continue;
		}

		component_start = input_position;
		while( normalized[input_position] != '\0' &&
		       normalized[input_position] != '\\' ) {
			if( normalized[input_position] == '*' || normalized[input_position] == '?' )
				has_wildcard = TRUE;
			input_position++;
		}
		component_length = input_position - component_start;

		if( unc_path && unc_components < 2u ) {
			unc_components++;
		} else if( !has_wildcard && component_length != 0u &&
		          !(component_length == 1u && normalized[component_start] == '.') &&
		          !(component_length == 2u && normalized[component_start] == '.' &&
		            normalized[component_start + 1u] == '.') ) {
			int valid_utf8 = fb_hIsValidUtf8(
				(const unsigned char *)normalized + component_start,
				component_length, &contains_non_ascii );
			encode_component = fb_hStartsWithWin9xPathPrefix(
				(const unsigned char *)normalized + component_start,
				component_length ) || (valid_utf8 && contains_non_ascii);
		}

		if( encode_component ) {
			static const char hex[] = "0123456789ABCDEF";
			size_t index;
			size_t escaped_length;

			/* The alias itself must fit in one VFAT long-file-name component. */
			if( component_length >
			    (FB_WIN9X_MAX_COMPONENT_LENGTH - FB_WIN9X_PATH_PREFIX_LENGTH) / 2u ) {
				free( result );
				free( normalized );
				errno = ENAMETOOLONG;
				return NULL;
			}
			escaped_length = FB_WIN9X_PATH_PREFIX_LENGTH + component_length * 2u;

			if( capacity - output_position <= FB_WIN9X_PATH_PREFIX_LENGTH ||
			    escaped_length >= capacity - output_position ) {
				free( result );
				free( normalized );
				errno = ENAMETOOLONG;
				return NULL;
			}

			memcpy( result + output_position, FB_WIN9X_PATH_PREFIX,
			        FB_WIN9X_PATH_PREFIX_LENGTH );
			output_position += FB_WIN9X_PATH_PREFIX_LENGTH;
			for( index = 0; index < component_length; ++index ) {
				unsigned char byte = (unsigned char)normalized[component_start + index];
				result[output_position++] = hex[byte >> 4];
				result[output_position++] = hex[byte & 0x0fu];
			}
		} else {
			if( component_length >= capacity - output_position ) {
				free( result );
				free( normalized );
				errno = ENAMETOOLONG;
				return NULL;
			}
			memcpy( result + output_position,
			        normalized + component_start, component_length );
			output_position += component_length;
		}
	}

	result[output_position] = '\0';
	free( normalized );
	return result;
}

char *fb_hConvertPathFromAC( const char *name )
{
	size_t name_length;
	size_t encoded_length;
	size_t decoded_length;
	size_t index;
	char *decoded;
	int contains_non_ascii;
	int starts_with_prefix;
	int valid_utf8;

	DBG_ASSERT( name != NULL );
	if( name == NULL ) {
		errno = EINVAL;
		return NULL;
	}

	name_length = strlen( name );
	starts_with_prefix = fb_hStartsWithWin9xPathPrefix(
		(const unsigned char *)name, name_length );
	if( !starts_with_prefix )
		return strdup( name );

	encoded_length = name_length - FB_WIN9X_PATH_PREFIX_LENGTH;
	if( encoded_length == 0u || (encoded_length & 1u) != 0u )
		return strdup( name );

	decoded_length = encoded_length / 2u;
	if( decoded_length == SIZE_MAX ) {
		errno = ENOMEM;
		return NULL;
	}
	decoded = (char*)malloc( decoded_length + 1u );
	if( decoded == NULL )
		return NULL;

	for( index = 0; index < decoded_length; ++index ) {
		int high = fb_hHexValue((unsigned char)name[FB_WIN9X_PATH_PREFIX_LENGTH + index * 2u]);
		int low = fb_hHexValue((unsigned char)name[FB_WIN9X_PATH_PREFIX_LENGTH + index * 2u + 1u]);
		if( high < 0 || low < 0 ) {
			free( decoded );
			return strdup( name );
		}
		decoded[index] = (char)((high << 4) | low);
		if( decoded[index] == '\0' || decoded[index] == '/' || decoded[index] == '\\' ) {
			free( decoded );
			return strdup( name );
		}
	}
	decoded[decoded_length] = '\0';

	valid_utf8 = fb_hIsValidUtf8((const unsigned char *)decoded, decoded_length,
	                             &contains_non_ascii);
	if( (!valid_utf8 && !fb_hStartsWithWin9xPathPrefix(
		(const unsigned char *)decoded, decoded_length)) ||
	    (valid_utf8 && !contains_non_ascii && !fb_hStartsWithWin9xPathPrefix(
		(const unsigned char *)decoded, decoded_length)) ) {
		free( decoded );
		return strdup( name );
	}

	return decoded;
}

static int fb_hCopyModeToWC( const char *mode, FB_WCHAR *wmode, size_t wmode_len )
{
	size_t i;

	if( mode == NULL || wmode == NULL || wmode_len == 0 ) {
		errno = EINVAL;
		return FALSE;
	}

	/* Keep the terminator inside the caller's buffer in release builds too.
	   The file entry points must reject a truncated mode before calling CRT. */
	for( i = 0; i < wmode_len - 1 && mode[i] != '\0'; ++i ) {
		wmode[i] = (FB_WCHAR)(unsigned char)mode[i];
	}
	wmode[i] = L'\0';
	if( mode[i] != '\0' ) {
		wmode[0] = L'\0';
		errno = EINVAL;
		return FALSE;
	}
	return TRUE;
}

static FB_WCHAR *fb_hConvertPathToWCInternal( const char *path, UINT codepage, DWORD flags )
{
	char *tmp;
	int chars;
	size_t path_len;
	FB_WCHAR *wpath;

	DBG_ASSERT( path != NULL );

	path_len = strlen( path ) + 1;
	tmp = (char*)malloc( path_len );
	if( tmp == NULL )
		return NULL;

	memcpy( tmp, path, path_len );
	fb_hConvertPath( tmp );

	chars = MultiByteToWideChar( codepage, flags, tmp, -1, NULL, 0 );
	if( chars <= 0 ) {
		free( tmp );
		return NULL;
	}
	if( (size_t)chars > SIZE_MAX / sizeof( FB_WCHAR ) ) {
		free( tmp );
		return NULL;
	}

	wpath = (FB_WCHAR*)malloc( (size_t)chars * sizeof( FB_WCHAR ) );
	if( wpath != NULL ) {
		if( MultiByteToWideChar( codepage, flags, tmp, -1, wpath, chars ) <= 0 ) {
			free( wpath );
			wpath = NULL;
		}
	}

	free( tmp );
	return wpath;
}

static FILE *fb_hOpenFileFromWC( const FB_WCHAR *path, const char *mode )
{
	FB_WCHAR wmode[8];

	DBG_ASSERT( path != NULL );
	DBG_ASSERT( mode != NULL );

	if( !fb_hCopyModeToWC( mode, wmode, ARRAY_SIZE( wmode ) ) )
		return NULL;
	return _wfopen( path, wmode );
}

FB_WCHAR *fb_hConvertPathToWC( const char *path, int *used_utf8 )
{
	FB_WCHAR *wpath;

	if( used_utf8 != NULL )
		*used_utf8 = FALSE;

	wpath = fb_hConvertPathToWCInternal( path, CP_UTF8, MB_ERR_INVALID_CHARS );
	if( wpath != NULL ) {
		if( used_utf8 != NULL )
			*used_utf8 = TRUE;
		return wpath;
	}

	return fb_hConvertPathToWCInternal( path, CP_ACP, 0 );
}

char *fb_hConvertPathFromWC( const FB_WCHAR *path, int use_utf8 )
{
	UINT codepage;
	int bytes;
	char *result;

	DBG_ASSERT( path != NULL );

	codepage = use_utf8 ? CP_UTF8 : CP_ACP;
	bytes = WideCharToMultiByte( codepage, 0, path, -1, NULL, 0, NULL, NULL );
	if( bytes <= 0 )
		return NULL;

	result = (char*)malloc( (size_t)bytes );
	if( result == NULL )
		return NULL;

	if( WideCharToMultiByte( codepage, 0, path, -1, result, bytes, NULL, NULL ) <= 0 ) {
		free( result );
		return NULL;
	}

	return result;
}

/* ------------------------------------------------------------------------- */
/* Filesystem operations                                                     */
/* ------------------------------------------------------------------------- */

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	char *apath;
	FILE *fp;
	FB_WCHAR *wpath;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return fopen( path, mode );

		fp = fopen( apath, mode );
		free( apath );
		return fp;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return fopen( path, mode );

	fp = fb_hOpenFileFromWC( wpath, mode );
	free( wpath );
	return fp;
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	char *apath;
	FB_WCHAR *wpath;
	FILE *fp;
	FB_WCHAR wmode[8];

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return freopen( path, mode, stream );

		fp = freopen( apath, mode, stream );
		free( apath );
		return fp;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return freopen( path, mode, stream );

	if( !fb_hCopyModeToWC( mode, wmode, ARRAY_SIZE( wmode ) ) ) {
		free( wpath );
		return NULL;
	}
	fp = _wfreopen( wpath, wmode, stream );
	free( wpath );
	return fp;
}

int fb_hStatFile( const char *path, struct _stat *buffer )
{
	char *apath;
	FB_WCHAR *wpath;
	int result;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return _stat( path, buffer );

		result = _stat( apath, buffer );
		free( apath );
		return result;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _stat( path, buffer );

	result = _wstat( wpath, buffer );
	free( wpath );
	return result;
}

int fb_hRemoveFile( const char *path )
{
	char *apath;
	FB_WCHAR *wpath;
	int result;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return remove( path );

		result = remove( apath );
		free( apath );
		return result;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return remove( path );

	result = _wremove( wpath );
	free( wpath );
	return result;
}

int fb_hMakeDir( const char *path )
{
	char *apath;
	FB_WCHAR *wpath;
	int result;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return _mkdir( path );

		result = _mkdir( apath );
		free( apath );
		return result;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _mkdir( path );

	result = _wmkdir( wpath );
	free( wpath );
	return result;
}

int fb_hChangeDir( const char *path )
{
	char *apath;
	FB_WCHAR *wpath;
	int result;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return _chdir( path );

		result = _chdir( apath );
		free( apath );
		return result;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _chdir( path );

	result = _wchdir( wpath );
	free( wpath );
	return result;
}

int fb_hRemoveDir( const char *path )
{
	char *apath;
	FB_WCHAR *wpath;
	int result;

	if( fb_hWin32IsWin9x( ) ) {
		apath = fb_hConvertPathToAC( path );
		if( apath == NULL )
			return _rmdir( path );

		result = _rmdir( apath );
		free( apath );
		return result;
	}

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _rmdir( path );

	result = _wrmdir( wpath );
	free( wpath );
	return result;
}
#else
FILE *fb_hOpenFile( const char *path, const char *mode )
{
	return fopen( path, mode );
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	return freopen( path, mode, stream );
}

int fb_hRemoveFile( const char *path )
{
	return remove( path );
}

int fb_hMakeDir( const char *path )
{
#ifdef HOST_MINGW
	return _mkdir( path );
#else
	return mkdir( path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
#endif
}

int fb_hChangeDir( const char *path )
{
#ifdef HOST_MINGW
	return _chdir( path );
#else
	return chdir( path );
#endif
}

int fb_hRemoveDir( const char *path )
{
#ifdef HOST_MINGW
	return _rmdir( path );
#else
	return rmdir( path );
#endif
}
#endif

/* end of drivers/win32/file_hconvpath.c */
