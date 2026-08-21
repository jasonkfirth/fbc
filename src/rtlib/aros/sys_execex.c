/*
    FreeBASIC runtime library
    -------------------------

    File: aros/sys_execex.c

    Purpose:

        Implement EXEC, RUN, and CHAIN through native AROS process services.

    Responsibilities:

        - parse FreeBASIC argument strings into an argv vector
        - run a child synchronously through dos.library for EXEC and CHAIN
        - replace the current program for RUN
        - restore console state when an execution attempt returns

    This file intentionally does NOT contain:

        - Unix fork() assumptions
        - shell command parsing
        - Workbench icon launching

    AROS process model:

        AROS does not implement fork().  SystemTags() provides the synchronous
        command execution that FreeBASIC needs without copying or temporarily
        borrowing the caller's stack, address space, or C-library state.
*/

#include "../fb.h"
#include "../unix/fb_private_console.h"

#include <dos/dostags.h>
#include <proto/dos.h>
#include <unistd.h>

FBCALL int fb_ExecEx( FBSTRING *program, FBSTRING *args, int do_fork )
{
	char buffer[MAX_PATH];
	char *command;
	char empty_arguments[] = "";
	char *arguments;
	char **argv;
	char *cursor;
	char *write_cursor;
	int argument_count = 0;
	int index;
	volatile int result = -1;
	size_t command_length;
	ssize_t arguments_length;

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
		arguments = alloca( arguments_length + 1 );
		DBG_ASSERT( arguments != NULL );
		arguments[arguments_length] = '\0';
		if( arguments_length > 0 )
			argument_count = fb_hParseArgs( arguments, args->data, arguments_length );
	}

	FB_STRLOCK();
	fb_hStrDelTemp_NoLock( args );
	fb_hStrDelTemp_NoLock( program );
	FB_STRUNLOCK();

	if( argument_count == -1 )
		return -1;

	argument_count++;
	argv = alloca( sizeof( char * ) * (argument_count + 1) );
	DBG_ASSERT( argv != NULL );

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
		command_length = strlen( buffer ) + 1;
		for( index = 1; index < argument_count; index++ )
			command_length += strlen( argv[index] ) + 3;
		command = alloca( command_length );
		DBG_ASSERT( command != NULL );
		write_cursor = command;
		write_cursor += snprintf( write_cursor, command_length, "%s", buffer );
		for( index = 1; index < argument_count; index++ ) {
			write_cursor += snprintf(
				write_cursor,
				command_length - (size_t)(write_cursor - command),
				" \"%s\"",
				argv[index]
			);
		}

		result = SystemTags(
			command,
			SYS_Input, Input(),
			SYS_Output, Output(),
			SYS_Error, Output(),
			SYS_Asynch, FALSE,
			TAG_DONE
		);
	} else {
		result = execvp( buffer, argv );
	}

	FB_LOCK();
	fb_hInitConsole();
	FB_UNLOCK();

	return result;
}

/* end of aros/sys_execex.c */
