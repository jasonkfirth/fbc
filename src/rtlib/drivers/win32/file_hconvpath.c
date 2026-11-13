/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/file_hconvpath.c

    Purpose:

        Adapt FreeBASIC path strings to the host filesystem interfaces.

    Responsibilities:

        - normalize directory separators
        - convert UTF-8 or system-codepage paths for Windows Unicode APIs
        - select filesystem calls supported by each desktop Windows family

    This file intentionally does NOT contain: - directory enumeration
        - file-handle bookkeeping
        - path searching or canonicalization
*/

#include "../../fb.h"
#include <errno.h>

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
	ssize_t i, len;

	DBG_ASSERT( path != NULL );

	len = strlen( path );
	for( i = 0; i < len; i++ )
	{
#if defined( HOST_DOS ) || defined( HOST_MINGW )
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

static char *fb_hConvertPathToAC( const char *path )
{
	char *result;
	size_t path_len;

	DBG_ASSERT( path != NULL );

	path_len = strlen( path ) + 1;
	result = (char*)malloc( path_len );
	if( result == NULL )
		return NULL;

	memcpy( result, path, path_len );
	fb_hConvertPath( result );
	return result;
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

	wpath = (FB_WCHAR*)malloc( chars * sizeof( FB_WCHAR ) );
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

	result = (char*)malloc( bytes );
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
