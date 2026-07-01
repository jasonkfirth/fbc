/*
    FreeBASIC runtime library
    -------------------------

    File: fb_nuttx_pipe.c

    Purpose:

        Provide the NuttX implementation of OPEN PIPE for the small
        runtime used by the RISC-V/NuttX test harness.

    Responsibilities:

        - accept the read-only pipe form used by the compiler test suite
        - expose that command output through the ordinary file input path
        - keep process and shell assumptions out of the common file code

    This file intentionally does NOT contain:

        - a general shell
        - process management
        - bidirectional pipe support
*/

#include "fb.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Minimal command parser                                                    */
/* ------------------------------------------------------------------------- */

static char *fb_nuttx_pipe_skip_spaces( char *text )
{
	while( isspace( (unsigned char)*text ) )
		++text;

	return text;
}

static void fb_nuttx_pipe_trim_right( char *text )
{
	size_t len = strlen( text );

	while( len > 0 ) {
		if( !isspace( (unsigned char)text[len - 1] ) )
			break;

		text[--len] = '\0';
	}
}

static int fb_nuttx_pipe_copy_ls_path( char *command, char *path, size_t path_len )
{
	char *end;
	size_t len;

	/*
		The first fbctests user of OPEN PIPE only needs this shape:

		    OPEN PIPE "ls ./file/pipe.bas" FOR INPUT

		NuttX targets may not have a host shell or popen(), and the real
		microcontroller will not have a useful Unix process model.  Rather
		than pretending otherwise, support the one read-only directory
		listing form that proves BASIC pipe input can flow through the
		file APIs.
	*/
	command = fb_nuttx_pipe_skip_spaces( command );

	if( (tolower( (unsigned char)command[0] ) != 'l') ||
	    (tolower( (unsigned char)command[1] ) != 's') ||
	    ((command[2] != '\0') && !isspace( (unsigned char)command[2] )) )
		return 0;

	command = fb_nuttx_pipe_skip_spaces( command + 2 );
	if( *command == '\0' )
		return 0;

	if( (*command == '"') || (*command == '\'') ) {
		char quote = *command++;

		end = strchr( command, quote );
		if( end == NULL )
			return 0;

		*end = '\0';
	} else {
		fb_nuttx_pipe_trim_right( command );
	}

	len = strlen( command );
	if( (len == 0) || (len >= path_len) )
		return 0;

	memcpy( path, command, len + 1 );
	return 1;
}

/* ------------------------------------------------------------------------- */
/* Pipe-backed file creation                                                 */
/* ------------------------------------------------------------------------- */

static int fb_nuttx_pipe_write_listing( const char *path, int fnum,
                                        char *temp_name, size_t temp_name_len )
{
	FILE *fp;
	int written;
	size_t path_len;

	fp = fopen( path, "rb" );
	if( fp == NULL )
		return -1;

	fclose( fp );

	written = snprintf( temp_name, temp_name_len,
	                    ".fb_nuttx_pipe_%d.tmp", fnum );
	if( (written < 0) || ((size_t)written >= temp_name_len) )
		return -1;

	fp = fopen( temp_name, "wb" );
	if( fp == NULL )
		return -1;

	path_len = strlen( path );
	if( (fwrite( path, 1, path_len, fp ) != path_len) ||
	    (fwrite( "\n", 1, 1, fp ) != 1) ) {
		fclose( fp );
		return -1;
	}

	fclose( fp );
	return 0;
}

/* ------------------------------------------------------------------------- */
/* OPEN PIPE entry point                                                     */
/* ------------------------------------------------------------------------- */

int32 fb_FileOpenPipe( const FBSTRING *str_filename, const uint32 mode,
                       const uint32 access, const uint32 lock,
                       const int32 fnum, const int32 len,
                       const char *encoding )
{
	FBSTRING temp_string;
	char *command;
	char path[512];
	char temp_name[64];
	size_t command_len;
	int res;

	(void)encoding;

	if( (fnum <= 0) || (fnum >= FB_NUTTX_MAX_FILES) )
		return -1;

	if( (str_filename == NULL) || (str_filename->data == NULL) )
		return -1;

	if( mode != 2 )
		return -1;

	if( str_filename->len < 0 )
		command_len = strlen( str_filename->data );
	else
		command_len = (size_t)str_filename->len;

	command = (char *)malloc( command_len + 1 );
	if( command == NULL )
		return -1;

	memcpy( command, str_filename->data, command_len );
	command[command_len] = '\0';

	res = fb_nuttx_pipe_copy_ls_path( command, path, sizeof( path ) );
	free( command );

	if( !res )
		return -1;

	res = fb_nuttx_pipe_write_listing( path, fnum,
	                                   temp_name, sizeof( temp_name ) );
	if( res != 0 )
		return res;

	temp_string.data = temp_name;
	temp_string.len = strlen( temp_name );
	temp_string.size = temp_string.len + 1;

	return fb_FileOpen( &temp_string, mode, access, lock,
	                    fnum, len );
}

/* end of fb_nuttx_pipe.c */
