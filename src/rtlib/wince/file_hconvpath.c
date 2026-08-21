/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_hconvpath.c

    Purpose:

        Normalize FreeBASIC paths and bridge them to Unicode Windows CE file
        and directory APIs.

    Responsibilities:

        - translate forward slashes to Windows CE separators
        - convert UTF-8 or active-code-page paths to UTF-16
        - wrap stdio and directory operations with checked allocations

    This file intentionally does NOT contain:

        - desktop drive enumeration
        - path search policy
        - file data transfer logic
*/

#include "../fb.h"

#include <limits.h>
#include <windows.h>
#include <sys/stat.h>
#include <wchar.h>

/* ------------------------------------------------------------------------- */
/* Path conversion                                                           */
/* ------------------------------------------------------------------------- */

static wchar_t current_directory[MAX_PATH + 1];

static void fb_hEnsureCurrentDirectory( void )
{
	wchar_t *separator;

	if( current_directory[0] != L'\0' )
		return;

	if( !fb_hWinCEGetExecutablePathWC( current_directory,
	                                  ARRAY_SIZE( current_directory ) ) ) {
		current_directory[0] = L'\\';
		current_directory[1] = L'\0';
		return;
	}

	separator = wcsrchr( current_directory, L'\\' );
	if( separator == NULL ) {
		current_directory[0] = L'\\';
		current_directory[1] = L'\0';
	} else if( separator == current_directory ) {
		separator[1] = L'\0';
	} else {
		*separator = L'\0';
	}
}

static wchar_t *fb_hResolveRelativePath( wchar_t *wide_path )
{
	wchar_t *resolved_path;
	size_t current_length;
	size_t path_length;
	size_t resolved_length;
	int needs_separator;

	if( (wide_path[0] == L'\\') || (wide_path[0] == L'/') ||
	    ((wide_path[0] != L'\0') && (wide_path[1] == L':')) ) {
		return wide_path;
	}

	FB_LOCK();
	fb_hEnsureCurrentDirectory();

	current_length = wcslen( current_directory );
	path_length = wcslen( wide_path );
	needs_separator = (current_length > 0) &&
	                  (current_directory[current_length - 1] != L'\\');
	resolved_length = current_length + (size_t)needs_separator + path_length;

	if( resolved_length > (SIZE_MAX / sizeof( wchar_t )) - 1 ) {
		FB_UNLOCK();
		free( wide_path );
		return NULL;
	}

	resolved_path = malloc( (resolved_length + 1) * sizeof( wchar_t ) );
	if( resolved_path != NULL ) {
		memcpy( resolved_path, current_directory,
		        current_length * sizeof( wchar_t ) );
		if( needs_separator )
			resolved_path[current_length++] = L'\\';
		memcpy( resolved_path + current_length, wide_path,
		        (path_length + 1) * sizeof( wchar_t ) );
	}

	FB_UNLOCK();
	free( wide_path );

	return resolved_path;
}

int fb_hWinCEGetCurrentDirectoryWC( wchar_t *destination,
	                                size_t destination_length )
{
	size_t length;

	if( (destination == NULL) || (destination_length == 0) )
		return FALSE;

	FB_LOCK();
	fb_hEnsureCurrentDirectory();
	length = wcslen( current_directory );
	if( length >= destination_length ) {
		FB_UNLOCK();
		return FALSE;
	}

	memcpy( destination, current_directory,
	        (length + 1) * sizeof( wchar_t ) );
	FB_UNLOCK();

	return TRUE;
}

void fb_hConvertPath( char *path )
{
	ssize_t index;
	ssize_t length;

	DBG_ASSERT( path != NULL );

	length = strlen( path );
	for( index = 0; index < length; ++index ) {
		if( path[index] == '/' )
			path[index] = '\\';
	}
}

static wchar_t *fb_hConvertToWCInternal( const char *text, UINT codepage,
	                                     DWORD flags, int is_path )
{
	char *copy;
	int chars;
	size_t text_length;
	wchar_t *wide_text;

	DBG_ASSERT( text != NULL );

	text_length = strlen( text ) + 1;
	copy = malloc( text_length );
	if( copy == NULL )
		return NULL;

	memcpy( copy, text, text_length );
	if( is_path )
		fb_hConvertPath( copy );

	chars = MultiByteToWideChar( codepage, flags, copy, -1, NULL, 0 );
	if( chars <= 0 ) {
		free( copy );
		return NULL;
	}

	if( (size_t)chars > (SIZE_MAX / sizeof( wchar_t )) ) {
		free( copy );
		return NULL;
	}

	wide_text = malloc( (size_t)chars * sizeof( wchar_t ) );
	if( wide_text != NULL ) {
		if( MultiByteToWideChar( codepage, flags, copy, -1, wide_text,
		                         chars ) <= 0 ) {
			free( wide_text );
			wide_text = NULL;
		}
	}

	free( copy );
	return wide_text;
}

wchar_t *fb_hWinCEToWC( const char *text )
{
	wchar_t *wide_text;

	if( text == NULL )
		return NULL;

	wide_text = fb_hConvertToWCInternal( text, CP_UTF8,
	                                    MB_ERR_INVALID_CHARS, FALSE );
	if( wide_text != NULL )
		return wide_text;

	return fb_hConvertToWCInternal( text, CP_ACP, 0, FALSE );
}

wchar_t *fb_hConvertPathToWC( const char *path, int *used_utf8 )
{
	wchar_t *wide_path;

	if( path == NULL )
		return NULL;

	if( used_utf8 != NULL )
		*used_utf8 = FALSE;

	wide_path = fb_hConvertToWCInternal( path, CP_UTF8,
	                                    MB_ERR_INVALID_CHARS, TRUE );
	if( wide_path != NULL ) {
		if( used_utf8 != NULL )
			*used_utf8 = TRUE;
		return fb_hResolveRelativePath( wide_path );
	}

	wide_path = fb_hConvertToWCInternal( path, CP_ACP, 0, TRUE );
	return (wide_path != NULL) ? fb_hResolveRelativePath( wide_path ) : NULL;
}

char *fb_hConvertPathFromWC( const wchar_t *path, int use_utf8 )
{
	UINT codepage;
	int bytes;
	char *result;

	if( path == NULL )
		return NULL;

	codepage = use_utf8 ? CP_UTF8 : CP_ACP;
	bytes = WideCharToMultiByte( codepage, 0, path, -1, NULL, 0, NULL, NULL );
	if( bytes <= 0 )
		return NULL;

	result = malloc( (size_t)bytes );
	if( result == NULL )
		return NULL;

	if( WideCharToMultiByte( codepage, 0, path, -1, result, bytes, NULL, NULL ) <= 0 ) {
		free( result );
		return NULL;
	}

	return result;
}

static void fb_hCopyModeToWC( const char *mode, wchar_t *wide_mode, size_t wide_mode_length )
{
	size_t index;

	DBG_ASSERT( mode != NULL );
	DBG_ASSERT( wide_mode != NULL );
	DBG_ASSERT( wide_mode_length >= 2 );

	for( index = 0; mode[index] != '\0'; ++index ) {
		if( index >= (wide_mode_length - 1) )
			break;
		wide_mode[index] = (wchar_t)(unsigned char)mode[index];
	}
	wide_mode[index] = L'\0';
}

/* ------------------------------------------------------------------------- */
/* File wrappers                                                             */
/* ------------------------------------------------------------------------- */

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	wchar_t wide_mode[8];
	wchar_t *wide_path;
	FILE *stream;

	if( path == NULL || mode == NULL )
		return NULL;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return NULL;

	fb_hCopyModeToWC( mode, wide_mode, ARRAY_SIZE( wide_mode ) );
	stream = _wfopen( wide_path, wide_mode );
	free( wide_path );
	return stream;
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	wchar_t wide_mode[8];
	wchar_t *wide_path;
	FILE *result;

	if( path == NULL || mode == NULL || stream == NULL )
		return NULL;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return NULL;

	fb_hCopyModeToWC( mode, wide_mode, ARRAY_SIZE( wide_mode ) );
	result = _wfreopen( wide_path, wide_mode, stream );
	free( wide_path );
	return result;
}

static time_t fb_hFileTimeToTime( const FILETIME *file_time )
{
	const unsigned long long epoch_offset = 116444736000000000ULL;
	unsigned long long ticks;

	ticks = ((unsigned long long)file_time->dwHighDateTime << 32) |
	        (unsigned long long)file_time->dwLowDateTime;
	if( ticks < epoch_offset )
		return (time_t)0;

	return (time_t)((ticks - epoch_offset) / 10000000ULL);
}

int fb_hStatFile( const char *path, struct _stat *buffer )
{
	wchar_t *wide_path;
	WIN32_FILE_ATTRIBUTE_DATA data;
	unsigned long long file_size;
	BOOL succeeded;

	if( path == NULL || buffer == NULL )
		return -1;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return -1;

	succeeded = GetFileAttributesExW( wide_path, GetFileExInfoStandard, &data );
	free( wide_path );
	if( !succeeded )
		return -1;

	memset( buffer, 0, sizeof( *buffer ) );
	if( (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 )
		buffer->st_mode = _S_IFDIR | _S_IREAD | _S_IEXEC;
	else
		buffer->st_mode = _S_IFREG | _S_IREAD;

	if( (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0 )
		buffer->st_mode |= _S_IWRITE;

	buffer->st_nlink = 1;
	file_size = ((unsigned long long)data.nFileSizeHigh << 32) |
	            (unsigned long long)data.nFileSizeLow;
	buffer->st_size = (file_size > LONG_MAX) ? LONG_MAX : (long)file_size;
	buffer->st_atime = fb_hFileTimeToTime( &data.ftLastAccessTime );
	buffer->st_mtime = fb_hFileTimeToTime( &data.ftLastWriteTime );
	buffer->st_ctime = fb_hFileTimeToTime( &data.ftCreationTime );

	return 0;
}

int fb_hRemoveFile( const char *path )
{
	wchar_t *wide_path;
	int result;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return -1;

	result = DeleteFileW( wide_path ) ? 0 : -1;
	free( wide_path );
	return result;
}

/* ------------------------------------------------------------------------- */
/* Directory wrappers                                                        */
/* ------------------------------------------------------------------------- */

static int fb_hWinceDirectoryOperation( const char *path, BOOL (WINAPI *operation)(LPCWSTR) )
{
	wchar_t *wide_path;
	BOOL result;

	if( operation == NULL )
		return -1;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return -1;

	result = operation( wide_path );
	free( wide_path );
	return result ? 0 : -1;
}

static BOOL WINAPI fb_hWinceCreateDirectory( LPCWSTR path )
{
	return CreateDirectoryW( path, NULL );
}

int fb_hMakeDir( const char *path )
{
	return fb_hWinceDirectoryOperation( path, fb_hWinceCreateDirectory );
}

int fb_hChangeDir( const char *path )
{
	wchar_t *wide_path;
	DWORD attributes;
	size_t length;
	int result = -1;

	wide_path = fb_hConvertPathToWC( path, NULL );
	if( wide_path == NULL )
		return -1;

	attributes = GetFileAttributesW( wide_path );
	if( (attributes != INVALID_FILE_ATTRIBUTES) &&
	    ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ) {
		length = wcslen( wide_path );
		if( length < ARRAY_SIZE( current_directory ) ) {
			FB_LOCK();
			memcpy( current_directory, wide_path,
			        (length + 1) * sizeof( wchar_t ) );
			FB_UNLOCK();
			result = 0;
		}
	}

	free( wide_path );
	return result;
}

int fb_hRemoveDir( const char *path )
{
	return fb_hWinceDirectoryOperation( path, RemoveDirectoryW );
}

/* end of wince/file_hconvpath.c */
