/* path conversion */

#include "../fb.h"
#include <errno.h>
#include <direct.h>

#define FB_XBOX_PATH_NO_PREFIX   0
#define FB_XBOX_PATH_TEMP_DRIVE  1
#define FB_XBOX_PATH_DISC_DRIVE  2

#define FB_XBOX_TEMP_DRIVE "E:\\"
#define FB_XBOX_DISC_DRIVE "D:\\"

static int hIsRelativePath( const char *path )
{
	if( (path[0] == '\0') || (path[1] == ':') || (path[0] == '\\') || (path[0] == '/') )
		return FALSE;

	return TRUE;
}

static int hModeCanWrite( const char *mode )
{
	const unsigned char *p = (const unsigned char *)mode;

	if( mode == NULL )
		return FALSE;

	while( *p != '\0' ) {
		if( (*p == 'w') || (*p == 'a') || (*p == '+') )
			return TRUE;
		++p;
	}

	return FALSE;
}

static int hNormalizeMode( const char *mode, char *normalized, size_t normalized_len )
{
	size_t i;
	size_t j = 0;

	if( (mode == NULL) || (normalized == NULL) || (normalized_len == 0) ) {
		errno = EINVAL;
		return FALSE;
	}

	for( i = 0; mode[i] != '\0'; i++ ) {
		if( mode[i] == 't' )
			continue;

		if( j >= (normalized_len - 1) ) {
			errno = EINVAL;
			return FALSE;
		}

		normalized[j++] = mode[i];
	}

	if( j == 0 ) {
		errno = EINVAL;
		return FALSE;
	}

	normalized[j] = '\0';
	return TRUE;
}

static int hModeHasFlag( const char *mode, char flag )
{
	const char *p = mode;

	while( *p != '\0' ) {
		if( *p == flag )
			return TRUE;
		++p;
	}

	return FALSE;
}

static FILE *hOpenDosFile( const char *path, const char *mode )
{
	FILE *fp;
	char normalized[8];
	const char *create_mode;
	const char *reopen_mode;

	if( !hNormalizeMode( mode, normalized, sizeof( normalized ) ) )
		return NULL;

	/*
		nxdk's PDCLib maps "w+" to an invalid CreateFileA disposition.  Create
		or truncate the file first, then reopen it read/write using the normal
		"r+" path.
	*/
	if( (normalized[0] == 'w') && hModeHasFlag( normalized, '+' ) ) {
		if( hModeHasFlag( normalized, 'b' ) ) {
			create_mode = "wb";
			reopen_mode = "r+b";
		} else {
			create_mode = "w";
			reopen_mode = "r+";
		}

		fp = fopen( path, create_mode );
		if( fp == NULL )
			return NULL;

		fclose( fp );
		return fopen( path, reopen_mode );
	}

	return fopen( path, normalized );
}

static int hEnsureParentDirs( const char *path )
{
	char partial[MAX_PATH];
	size_t i, len;

	len = strlen( path );
	if( len >= sizeof( partial ) ) {
		errno = ENAMETOOLONG;
		return FALSE;
	}

	memcpy( partial, path, len + 1 );

	for( i = 0; i < len; i++ ) {
		if( (partial[i] != '\\') && (partial[i] != '/') )
			continue;

		/* Skip the root separator in paths such as E:\foo. */
		if( (i == 2) && (partial[1] == ':') )
			continue;

		partial[i] = '\0';
		if( partial[0] != '\0' ) {
			if( (_mkdir( partial ) != 0) && (errno != EEXIST) )
				return FALSE;
		}
		partial[i] = '\\';
	}

	return TRUE;
}

static int hMakeDosPath
	(
		const char *path,
		char *dst,
		size_t dstlen,
		int path_prefix,
		const char **out_path
	)
{
	size_t prefix_len = 0;
	size_t path_len;
	const char *prefix = "";
	const char *path_tail = path;
	char dos_path[MAX_PATH];

	if( path == NULL ) {
		errno = EINVAL;
		return FALSE;
	}

	if( (path[0] == '\0') || (path[1] == ':') || (path[0] == '\\') || (path[0] == '/') ) {
		prefix = "";
		prefix_len = 0;
	} else {
		if( path_prefix == FB_XBOX_PATH_TEMP_DRIVE ) {
			prefix = FB_XBOX_TEMP_DRIVE;
			prefix_len = sizeof( FB_XBOX_TEMP_DRIVE ) - 1;
		} else if( path_prefix == FB_XBOX_PATH_DISC_DRIVE ) {
			prefix = FB_XBOX_DISC_DRIVE;
			prefix_len = sizeof( FB_XBOX_DISC_DRIVE ) - 1;
		}
	}

	/*
		PDCLib's Xbox file path layer accepts D:\file and E:\file paths,
		but it does not reliably collapse a leading .\ segment after a
		drive prefix.  The test suite commonly opens paths such as
		"./file/name.bas", so drop that no-op segment while building the
		drive-qualified path.
	*/
	if( (prefix_len > 0) && (path_tail[0] == '.') &&
	    ((path_tail[1] == '\\') || (path_tail[1] == '/')) ) {
		path_tail += 2;
	}

	path_len = strlen( path_tail );
	if( (sizeof( dos_path ) <= (prefix_len + 1)) ||
	    (dstlen <= (prefix_len + 1)) ||
	    (path_len > (sizeof( dos_path ) - prefix_len - 1)) ||
	    (path_len > (dstlen - prefix_len - 1)) ) {
		errno = ENAMETOOLONG;
		return FALSE;
	}

	memcpy( dos_path, prefix, prefix_len );
	memcpy( dos_path + prefix_len, path_tail, path_len + 1 );
	fb_hConvertPath( dos_path );

	strcpy( dst, dos_path );
	*out_path = dst;
	return TRUE;
}

static FILE *hOpenConvertedFile( const char *path, const char *mode, int path_prefix )
{
	char dos_path[MAX_PATH];
	const char *open_path;

	if( !hMakeDosPath( path, dos_path, sizeof( dos_path ), path_prefix, &open_path ) )
		return NULL;

	if( (path_prefix == FB_XBOX_PATH_TEMP_DRIVE) && hModeCanWrite( mode ) ) {
		if( !hEnsureParentDirs( open_path ) )
			return NULL;
	}

	return hOpenDosFile( open_path, mode );
}

void fb_hConvertPath( char *path )
{
	ssize_t i, len;

	DBG_ASSERT( path != NULL );

	len = strlen( path );
	for( i = 0; i < len; i++ )
	{
		if( path[i] == '/' )
			path[i] = '\\';
	}
}

FILE *fb_hOpenFile( const char *path, const char *mode )
{
	FILE *fp;
	int is_relative;

	if( (path == NULL) || (mode == NULL) ) {
		errno = EINVAL;
		return NULL;
	}

	is_relative = hIsRelativePath( path );
	if( is_relative && hModeCanWrite( mode ) )
		return hOpenConvertedFile( path, mode, FB_XBOX_PATH_TEMP_DRIVE );

	if( is_relative )
		fp = hOpenConvertedFile( path, mode, FB_XBOX_PATH_DISC_DRIVE );
	else
		fp = hOpenConvertedFile( path, mode, FB_XBOX_PATH_NO_PREFIX );

	if( (fp == NULL) && is_relative )
		fp = hOpenConvertedFile( path, mode, FB_XBOX_PATH_NO_PREFIX );
	if( (fp == NULL) && is_relative )
		fp = hOpenConvertedFile( path, mode, FB_XBOX_PATH_TEMP_DRIVE );

	return fp;
}

FILE *fb_hReopenFile( const char *path, const char *mode, FILE *stream )
{
	if( stream != NULL )
		fclose( stream );

	return fb_hOpenFile( path, mode );
}

int fb_hRemoveFile( const char *path )
{
	char dos_path[MAX_PATH];
	const char *remove_path;
	int err;

	if( path == NULL ) {
		errno = EINVAL;
		return -1;
	}

	if( hIsRelativePath( path ) ) {
		if( hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_TEMP_DRIVE, &remove_path ) ) {
			if( remove( remove_path ) == 0 )
				return 0;
		}

		err = errno;
		if( hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_DISC_DRIVE, &remove_path ) ) {
			if( remove( remove_path ) == 0 )
				return 0;
		}

		if( err != 0 )
			errno = err;
		return -1;
	}

	if( !hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_NO_PREFIX, &remove_path ) )
		return -1;

	return remove( remove_path );
}

int fb_hMakeDir( const char *path )
{
	char dos_path[MAX_PATH];
	const char *mkdir_path;

	if( path == NULL ) {
		errno = EINVAL;
		return -1;
	}

	if( !hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_TEMP_DRIVE, &mkdir_path ) )
		return -1;

	return _mkdir( mkdir_path );
}

int fb_hChangeDir( const char *path )
{
	char dos_path[MAX_PATH];
	const char *chdir_path;

	if( path == NULL ) {
		errno = EINVAL;
		return -1;
	}

	if( !hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_DISC_DRIVE, &chdir_path ) )
		return -1;

	return _chdir( chdir_path );
}

int fb_hRemoveDir( const char *path )
{
	char dos_path[MAX_PATH];
	const char *rmdir_path;

	if( path == NULL ) {
		errno = EINVAL;
		return -1;
	}

	if( !hMakeDosPath( path, dos_path, sizeof( dos_path ), FB_XBOX_PATH_TEMP_DRIVE, &rmdir_path ) )
		return -1;

	return _rmdir( rmdir_path );
}
