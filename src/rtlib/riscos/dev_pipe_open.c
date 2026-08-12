/*
 * Project: FreeBASIC RISC OS runtime
 * ----------------------------------
 *
 * File: dev_pipe_open.c
 *
 * Purpose:
 *
 *     Provide a bounded OPEN PIPE input path without UnixLib popen().
 *
 * Responsibilities:
 *
 *     - recognize the read-only "ls <file>" command form
 *     - validate that the requested file exists
 *     - expose the resulting one-line listing through normal file hooks
 *     - reject unsupported modes and commands without starting a child task
 *
 * This file intentionally does NOT emulate a general-purpose shell. UnixLib's
 * popen() implementation documents that its child process association is
 * incomplete and can hang while waiting for the child.
 */

#include "../fb.h"

#include <ctype.h>

#define FB_RISCOS_PIPE_TEMP_FILE "fbpipe"

static FB_FILE_HOOKS hooks_dev_riscos_pipe = {
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

static char *hSkipSpaces( char *cursor )
{
	while( isspace( (unsigned char)*cursor ) )
		++cursor;

	return cursor;
}

static void hTrimRight( char *text )
{
	size_t length = strlen( text );

	while( length > 0 ) {
		if( !isspace( (unsigned char)text[length - 1] ) )
			break;

		text[--length] = '\0';
	}
}

static int hCopyListPath( char *command, char *destination,
					  size_t destination_length )
{
	char *path;
	char *end;
	size_t length;

	command = hSkipSpaces( command );
	if( (tolower( (unsigned char)command[0] ) != 'l') ||
	    (tolower( (unsigned char)command[1] ) != 's') ||
	    ((command[2] != '\0') && !isspace( (unsigned char)command[2] )) )
		return FALSE;

	path = hSkipSpaces( command + 2 );
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

	length = strlen( path );
	if( (length == 0) || (length >= destination_length) )
		return FALSE;

	memcpy( destination, path, length + 1 );
	return TRUE;
}

static int hOpenResult( FB_FILE *handle )
{
	FILE *stream;

	stream = fb_hOpenFile( FB_RISCOS_PIPE_TEMP_FILE, "rb" );
	if( stream == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	fb_hSetFileBufSize( stream );
	handle->size = fb_DevFileGetSize( stream, FB_FILE_MODE_INPUT,
								  handle->encod, FALSE );
	if( handle->size == -1 ) {
		fclose( stream );
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	stream = fb_hReopenFile( FB_RISCOS_PIPE_TEMP_FILE, "rt", stream );
	if( stream == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

	fb_hSetFileBufSize( stream );
	fb_hDevFileSeekStart( stream, FB_FILE_MODE_INPUT, handle->encod, FALSE );

	handle->hooks = &hooks_dev_riscos_pipe;
	handle->opaque = stream;
	handle->type = FB_FILE_TYPE_PIPE;
	handle->access = FB_FILE_ACCESS_READ;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_DevPipeOpen( FB_FILE *handle, const char *filename,
					size_t filename_length )
{
	char *command;
	char path[MAX_PATH];
	FILE *stream;
	size_t path_length;
	int valid_command;
	int result;

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

	command = (char *)malloc( filename_length + 1 );
	if( command == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	memcpy( command, filename, filename_length );
	command[filename_length] = '\0';
	valid_command = hCopyListPath( command, path, sizeof( path ) );
	free( command );

	if( !valid_command ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	stream = fb_hOpenFile( path, "rb" );
	if( stream == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	}
	fclose( stream );

	stream = fb_hOpenFile( FB_RISCOS_PIPE_TEMP_FILE, "wb" );
	if( stream == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	path_length = strlen( path );
	if( (fwrite( path, 1, path_length, stream ) != path_length) ||
	    (fwrite( "\n", 1, 1, stream ) != 1) ) {
		fclose( stream );
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	fclose( stream );

	result = hOpenResult( handle );
	FB_UNLOCK();
	return result;
}

/* end of dev_pipe_open.c */
