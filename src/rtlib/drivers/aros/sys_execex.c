/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/sys_execex.c

    Purpose:

        Implement EXEC, RUN, and CHAIN through native AROS process services.

    Responsibilities:

        - parse FreeBASIC argument strings into an argv vector
        - run a child synchronously through dos.library for EXEC and CHAIN
        - replace the current program for RUN
        - restore console state when an execution attempt returns

    This file intentionally does NOT contain: - Unix fork() assumptions
        - shell command parsing
        - Workbench icon launching

    AROS process model:

        AROS does not implement fork().  SystemTags() provides the synchronous
        command execution that FreeBASIC needs without copying or temporarily
        borrowing the caller's stack, address space, or C-library state.
*/

#include "../../fb.h"
#include "../unix/fb_private_console.h"

#include <dos/dostags.h>
#include <proto/dos.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>

FBCALL int fb_ExecEx( FBSTRING_ACCESS *program, FBSTRING_ACCESS *args, int do_fork )
{
	char buffer[MAX_PATH];
	char *command = NULL;
	char empty_arguments[] = "";
	char *arguments;
	char **argv = NULL;
	char *cursor;
	char *write_cursor;
	int argument_count = 0;
	int index;
	volatile int result = -1;
	size_t command_length;
	ssize_t arguments_length;
	int owns_arguments = FALSE;

	if( program == NULL || program->data == NULL ) {
		fb_hStrDelTemp( args );
		fb_hStrDelTemp( program );
		return -1;
	}

	strncpy( buffer, program->data, sizeof( buffer ) );
	buffer[sizeof( buffer ) - 1] = '\0';
	fb_hConvertPath( buffer );

	if( args == NULL ) {
		arguments = empty_arguments;
	} else {
		arguments_length = FB_STRSIZE( args );
		if( (arguments_length < 0) ||
		    ((args->data == NULL) && (arguments_length != 0)) ) {
			fb_hStrDelTemp( args );
			fb_hStrDelTemp( program );
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}
		if( arguments_length == 0 ) {
			arguments = empty_arguments;
		} else {
			arguments = (char *)malloc( (size_t)arguments_length + 1u );
			if( arguments == NULL ) {
				fb_hStrDelTemp( args );
				fb_hStrDelTemp( program );
				return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			}
			owns_arguments = TRUE;
			memcpy( arguments, args->data, (size_t)arguments_length );
			arguments[arguments_length] = '\0';
		}
		if( arguments_length > 0 )
			argument_count = fb_hParseArgs( arguments, args->data, arguments_length );
	}

	FB_STRLOCK();
	fb_hStrDelTemp_NoLock( args );
	fb_hStrDelTemp_NoLock( program );
	FB_STRUNLOCK();

	if( argument_count < 0 ) {
		if( owns_arguments )
			free( arguments );
		return -1;
	}
	if( argument_count == INT_MAX ) {
		if( owns_arguments )
			free( arguments );
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	argument_count++;
	if( ((size_t)argument_count + 1u) >
	    (SIZE_MAX / sizeof( argv[0] )) ) {
		if( owns_arguments )
			free( arguments );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	argv = (char **)malloc( ((size_t)argument_count + 1u) *
	                       sizeof( argv[0] ) );
	if( argv == NULL ) {
		if( owns_arguments )
			free( arguments );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	argv[0] = buffer;
	cursor = arguments;
	for( index = 1; index < argument_count; index++ ) {
		argv[index] = cursor;
		while( *cursor != '\0' )
			cursor++;
		cursor++;
	}
	argv[argument_count] = NULL;

	FB_LOCK();
	fb_hExitConsole();
	FB_UNLOCK();

	if( do_fork ) {
		/*
		 * The POSIX vfork() emulation temporarily runs child setup on the
		 * parent's context.  Tool drivers exercise enough C-library and file
		 * descriptor state during that interval to make the bridge unreliable,
		 * particularly on ARM.  SystemTags() is the native synchronous process
		 * boundary and preserves the command's return code.
		 */
		command_length = strlen( buffer ) + 1u;
		for( index = 1; index < argument_count; index++ ) {
			size_t argument_length = strlen( argv[index] );
			if( (command_length > (SIZE_MAX - 3u)) ||
			    (argument_length > (SIZE_MAX - command_length - 3u)) ) {
				result = fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
				goto restore_console;
			}
			command_length += argument_length + 3u;
		}
		command = (char *)malloc( command_length );
		if( command == NULL ) {
			result = fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			goto restore_console;
		}
		write_cursor = command;
		{
			size_t program_length = strlen( buffer );
			memcpy( write_cursor, buffer, program_length );
			write_cursor += program_length;
		}
		for( index = 1; index < argument_count; index++ ) {
			size_t argument_length = strlen( argv[index] );
			*write_cursor++ = ' ';
			*write_cursor++ = '"';
			memcpy( write_cursor, argv[index], argument_length );
			write_cursor += argument_length;
			*write_cursor++ = '"';
		}
		*write_cursor = '\0';

		result = SystemTags(
			(CONST_STRPTR)command,
			SYS_Input, Input(),
			SYS_Output, Output(),
			SYS_Error, Output(),
			SYS_Asynch, FALSE,
			TAG_DONE
		);
	} else {
		result = execvp( buffer, argv );
	}

restore_console:
	FB_LOCK();
	fb_hInitConsole();
	FB_UNLOCK();
	free( command );
	free( argv );
	if( owns_arguments )
		free( arguments );

	return result;
}

/* end of drivers/aros/sys_execex.c */
