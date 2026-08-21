/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_hshell.c

    Purpose:

        Implement SHELL on systems without a Windows CE command processor.

    Responsibilities:

        - split a quoted or unquoted executable from its arguments
        - launch the executable through CreateProcessW
        - wait for completion and return its exit code

    This file intentionally does NOT contain:

        - desktop cmd.exe integration
        - shell built-in commands
        - asynchronous process management
*/

#include "../fb.h"

#include <ctype.h>
#include <windows.h>

int fb_hShell( char *command )
{
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;
	const char *program_begin;
	const char *program_end;
	const char *arguments;
	char *program;
	wchar_t *wide_program;
	wchar_t *wide_arguments = NULL;
	size_t program_length;
	DWORD exit_code;
	int result = -1;

	if( command == NULL )
		return -1;

	program_begin = command;
	while( isspace( (unsigned char)*program_begin ) )
		++program_begin;

	if( *program_begin == '\0' )
		return -1;

	if( *program_begin == '"' ) {
		++program_begin;
		program_end = strchr( program_begin, '"' );
		if( program_end == NULL )
			program_end = program_begin + strlen( program_begin );
	} else {
		program_end = program_begin;
		while( (*program_end != '\0') &&
		       !isspace( (unsigned char)*program_end ) ) {
			++program_end;
		}
	}

	program_length = (size_t)(program_end - program_begin);
	if( program_length == 0 )
		return -1;

	program = malloc( program_length + 1 );
	if( program == NULL )
		return -1;

	memcpy( program, program_begin, program_length );
	program[program_length] = '\0';

	arguments = program_end;
	if( *arguments == '"' )
		++arguments;
	while( isspace( (unsigned char)*arguments ) )
		++arguments;

	wide_program = fb_hConvertPathToWC( program, NULL );
	free( program );
	if( wide_program == NULL )
		return -1;

	if( *arguments != '\0' ) {
		wide_arguments = fb_hWinCEToWC( arguments );
		if( wide_arguments == NULL )
			goto cleanup;
	}

	memset( &startup_info, 0, sizeof( startup_info ) );
	memset( &process_info, 0, sizeof( process_info ) );
	startup_info.cb = sizeof( startup_info );

	if( !CreateProcessW( wide_program, wide_arguments, NULL, NULL, FALSE, 0,
	                     NULL, NULL, &startup_info, &process_info ) ) {
		goto cleanup;
	}

	if( process_info.hThread != NULL )
		CloseHandle( process_info.hThread );

	if( (WaitForSingleObject( process_info.hProcess, INFINITE ) ==
	     WAIT_OBJECT_0) &&
	    GetExitCodeProcess( process_info.hProcess, &exit_code ) ) {
		result = (int)exit_code;
	}

	CloseHandle( process_info.hProcess );

cleanup:
	free( wide_arguments );
	free( wide_program );

	return result;
}

/* end of wince/sys_hshell.c */
