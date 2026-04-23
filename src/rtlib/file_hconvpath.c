/* path conversion */

#include "fb.h"

#ifdef HOST_MINGW
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

void fb_hConvertPath( char *path )
{
	ssize_t i, len;

	DBG_ASSERT( path != NULL );

	len = strlen( path );
	for( i = 0; i < len; i++ )
	{
#if defined( HOST_DOS ) || defined( HOST_MINGW ) || defined( HOST_XBOX )
		if( path[i] == '/' )
			path[i] = '\\';
#else
		if( path[i] == '\\' )
			path[i] = '/';
#endif
	}
}

#ifdef HOST_MINGW
static void fb_hCopyModeToWC( const char *mode, wchar_t *wmode, size_t wmode_len )
{
	size_t i;

	DBG_ASSERT( mode != NULL );
	DBG_ASSERT( wmode != NULL );
	DBG_ASSERT( wmode_len >= 2 );

	for( i = 0; mode[i] != '\0'; ++i ) {
		DBG_ASSERT( i < (wmode_len - 1) );
		wmode[i] = (wchar_t)(unsigned char)mode[i];
	}
	wmode[i] = L'\0';
}

static wchar_t *fb_hConvertPathToWCInternal( const char *path, UINT codepage, DWORD flags )
{
	char *tmp;
	int chars;
	wchar_t *wpath;

	DBG_ASSERT( path != NULL );

	tmp = (char*)malloc( strlen( path ) + 1 );
	if( tmp == NULL )
		return NULL;

	strcpy( tmp, path );
	fb_hConvertPath( tmp );

	chars = MultiByteToWideChar( codepage, flags, tmp, -1, NULL, 0 );
	if( chars <= 0 ) {
		free( tmp );
		return NULL;
	}

	wpath = (wchar_t*)malloc( chars * sizeof( wchar_t ) );
	if( wpath != NULL ) {
		if( MultiByteToWideChar( codepage, flags, tmp, -1, wpath, chars ) <= 0 ) {
			free( wpath );
			wpath = NULL;
		}
	}

	free( tmp );
	return wpath;
}

static FILE *fb_hOpenFileFromWC( const wchar_t *path, const char *mode )
{
	wchar_t wmode[8];

	DBG_ASSERT( path != NULL );
	DBG_ASSERT( mode != NULL );

	fb_hCopyModeToWC( mode, wmode, ARRAY_SIZE( wmode ) );
	return _wfopen( path, wmode );
}

wchar_t *fb_hConvertPathToWC( const char *path, int *used_utf8 )
{
	wchar_t *wpath;

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

char *fb_hConvertPathFromWC( const wchar_t *path, int use_utf8 )
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

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	FILE *fp;
	wchar_t *wpath;

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return fopen( path, mode );

	fp = fb_hOpenFileFromWC( wpath, mode );
	free( wpath );
	return fp;
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	wchar_t *wpath;
	FILE *fp;
	wchar_t wmode[8];

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return freopen( path, mode, stream );

	fb_hCopyModeToWC( mode, wmode, ARRAY_SIZE( wmode ) );
	fp = _wfreopen( wpath, wmode, stream );
	free( wpath );
	return fp;
}

int fb_hStatFile( const char *path, struct _stat *buffer )
{
	wchar_t *wpath;
	int result;

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _stat( path, buffer );

	result = _wstat( wpath, buffer );
	free( wpath );
	return result;
}

int fb_hRemoveFile( const char *path )
{
	wchar_t *wpath;
	int result;

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return remove( path );

	result = _wremove( wpath );
	free( wpath );
	return result;
}

int fb_hMakeDir( const char *path )
{
	wchar_t *wpath;
	int result;

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _mkdir( path );

	result = _wmkdir( wpath );
	free( wpath );
	return result;
}

int fb_hChangeDir( const char *path )
{
	wchar_t *wpath;
	int result;

	wpath = fb_hConvertPathToWC( path, NULL );
	if( wpath == NULL )
		return _chdir( path );

	result = _wchdir( wpath );
	free( wpath );
	return result;
}

int fb_hRemoveDir( const char *path )
{
	wchar_t *wpath;
	int result;

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
