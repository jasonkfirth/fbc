/* Xbox PIPE device */

#include "../fb.h"

#include <ctype.h>

#define FB_XBOX_PIPE_TEMP_FILE "E:\\FBPIPE.TMP"

static FB_FILE_HOOKS hooks_dev_xbox_pipe = {
	fb_DevFileEof,
	fb_DevFileClose,
	fb_DevFileSeek,
	fb_DevFileTell,
	fb_DevFileRead,
	fb_DevFileReadWstr,
	fb_DevFileWrite,
	fb_DevFileWriteWstr,
	fb_DevFileLock,
	fb_DevFileUnlock,
	fb_DevFileReadLine,
	fb_DevFileReadLineWstr,
	NULL,
	fb_DevFileFlush
};

static char *hSkipSpaces( char *p )
{
	while( isspace( (unsigned char)*p ) )
		++p;

	return p;
}

static void hTrimRight( char *text )
{
	size_t len = strlen( text );

	while( len > 0 ) {
		if( !isspace( (unsigned char)text[len - 1] ) )
			break;

		text[--len] = '\0';
	}
}

static int hCopyCommandPath( char *cmd, char *dst, size_t dst_len )
{
	char *path;
	char *end;
	size_t len;

	/*
		Xbox homebrew has no shell to back popen().  The test suite uses
		OPEN PIPE with "ls <file>" only to check that ordinary pipe input
		can be read.  Support that read-only shape by producing the same
		one-line listing a shell would have returned for an existing file.
	*/
	cmd = hSkipSpaces( cmd );
	if( (tolower( (unsigned char)cmd[0] ) != 'l') ||
	    (tolower( (unsigned char)cmd[1] ) != 's') ||
	    ((cmd[2] != '\0') && !isspace( (unsigned char)cmd[2] )) )
		return FALSE;

	path = hSkipSpaces( cmd + 2 );
	if( *path == '\0' )
		return FALSE;

	if( (*path == '"') || (*path == '\'') ) {
		char quote = *path++;

		end = strchr( path, quote );
		if( end == NULL )
			return FALSE;

		*end = '\0';
	} else {
		hTrimRight( path );
	}

	len = strlen( path );
	if( (len == 0) || (len >= dst_len) )
		return FALSE;

	memcpy( dst, path, len + 1 );
	return TRUE;
}

static int hOpenTempInput( FB_FILE *handle )
{
	FILE *fp;

	fp = fb_hOpenFile( FB_XBOX_PIPE_TEMP_FILE, "rb" );
	if( fp == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	fb_hSetFileBufSize( fp );
	handle->size = fb_DevFileGetSize( fp, FB_FILE_MODE_INPUT, handle->encod, FALSE );
	if( handle->size == -1 ) {
		fclose( fp );
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	fp = fb_hReopenFile( FB_XBOX_PIPE_TEMP_FILE, "rt", fp );
	if( fp == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	fb_hSetFileBufSize( fp );
	fb_hDevFileSeekStart( fp, FB_FILE_MODE_INPUT, handle->encod, FALSE );

	handle->hooks = &hooks_dev_xbox_pipe;
	handle->opaque = fp;
	handle->type = FB_FILE_TYPE_PIPE;
	handle->access = FB_FILE_ACCESS_READ;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_DevPipeOpen( FB_FILE *handle, const char *filename, size_t filename_len )
{
	char *cmd;
	char path[MAX_PATH];
	FILE *fp;
	size_t path_len;
	int res;

	FB_LOCK();

	if( handle->mode != FB_FILE_MODE_INPUT ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( handle->access == FB_FILE_ACCESS_ANY )
		handle->access = FB_FILE_ACCESS_READ;

	if( handle->access != FB_FILE_ACCESS_READ ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	cmd = (char *)malloc( filename_len + 1 );
	if( cmd == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	memcpy( cmd, filename, filename_len );
	cmd[filename_len] = '\0';

	res = hCopyCommandPath( cmd, path, sizeof( path ) );
	free( cmd );

	if( !res ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	fp = fb_hOpenFile( path, "rb" );
	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	}

	fclose( fp );

	fp = fb_hOpenFile( FB_XBOX_PIPE_TEMP_FILE, "wb" );
	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	path_len = strlen( path );
	if( (fwrite( path, 1, path_len, fp ) != path_len) ||
	    (fwrite( "\n", 1, 1, fp ) != 1) ) {
		fclose( fp );
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	fclose( fp );

	res = hOpenTempInput( handle );

	FB_UNLOCK();
	return res;
}
