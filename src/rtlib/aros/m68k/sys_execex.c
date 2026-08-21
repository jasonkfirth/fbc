/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/sys_execex.c

    Purpose:

        Implement EXEC, RUN, and CHAIN for native AROS m68k programs.

    Responsibilities:

        - parse FreeBASIC argument strings into an argv vector
        - run child programs in independent AROS processes
        - preserve stdin when shareable and retry compiler-style children
          without an incompatible CLI input handle
        - exclude unsafe copied shell-local-variable lists from child teardown
        - retain and safely release every loaded child segment
        - release completed children through AROS's child lifecycle API
        - replace the current program for RUN
        - restore console state when an execution attempt returns

    This file intentionally does NOT contain:

        - Unix fork() assumptions
        - shell command parsing
        - process policy for non-m68k AROS targets

    AROS m68k process model:

        Re-entering POSIXC in a compiler subprocess is unsafe, while the
        shell-mediated SystemTags path cannot reliably launch the native GCC
        toolchain.  CreateNewProcTags supplies an independent address space.
        NP_NotifyOnDeath retains a completed child's ETask until its parent
        calls ChildWait and ChildFree.  This avoids both raw signal races and
        NP_Synchronous's migration of interactive input handles.  The parent
        retains segment ownership and unloads the segment after ChildWait,
        while the completed child task record still exists.  ChildFree must
        come last: child-owned cleanup faults for converted Hunk tools, while
        unloading after ChildFree corrupts the m68k allocator.
*/

#include "../../fb.h"
#include "../../unix/fb_private_console.h"

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <unistd.h>

struct fb_aros_child_result
{
	volatile LONG result;
};

static void fb_hArosChildExit( IPTR data, IPTR result )
{
	struct fb_aros_child_result *child_result;

	/* AROS m68k's DosEntry stack shim exposes D1 before D0 to C code. */
	child_result = (struct fb_aros_child_result *)data;
	child_result->result = (LONG)result;
}

static char *fb_hArosArguments( char **argv, int argument_count )
{
	char *arguments;
	char *write_cursor;
	int index;
	size_t size = 2;

	for( index = 1; index < argument_count; index++ ) {
		const char *read_cursor;

		size += strlen( argv[index] ) + 3;
		for( read_cursor = argv[index]; *read_cursor != '\0'; read_cursor++ ) {
			if( *read_cursor == '\n' || *read_cursor == '"' || *read_cursor == '*' )
				size++;
		}
	}

	arguments = malloc( size );
	if( arguments == NULL )
		return NULL;
	write_cursor = arguments;
	for( index = 1; index < argument_count; index++ ) {
		const char *read_cursor;

		if( index != 1 )
			*write_cursor++ = ' ';
		*write_cursor++ = '"';

		for( read_cursor = argv[index]; *read_cursor != '\0'; read_cursor++ ) {
			if( *read_cursor == '\n' || *read_cursor == '"' || *read_cursor == '*' ) {
				*write_cursor++ = '*';
				*write_cursor++ = *read_cursor == '\n' ? 'N' : *read_cursor;
			} else {
				*write_cursor++ = *read_cursor;
			}
		}

		*write_cursor++ = '"';
	}

	*write_cursor++ = '\n';
	*write_cursor = '\0';
	return arguments;
}

static struct Process *fb_hArosCreateChild(
	BPTR segment,
	char *arguments,
	BPTR child_input,
	BPTR child_output,
	BPTR error_output,
	ULONG stack_size,
	const char *program,
	struct fb_aros_child_result *child_result
)
{
	return CreateNewProcTags(
		NP_Seglist, (IPTR)segment,
		NP_FreeSeglist, FALSE,
		NP_Arguments, (IPTR)arguments,
		NP_Input, (IPTR)child_input,
		NP_CloseInput, FALSE,
		NP_Output, (IPTR)child_output,
		NP_CloseOutput, FALSE,
		NP_Error, (IPTR)error_output,
		NP_CloseError, FALSE,
		NP_StackSize, (IPTR)stack_size,
		NP_Name, (IPTR)program,
		NP_Cli, TRUE,
		NP_CommandName, (IPTR)program,
		NP_ExitCode, (IPTR)fb_hArosChildExit,
		NP_ExitData, (IPTR)child_result,
		NP_CopyVars, FALSE,
		NP_NotifyOnDeath, TRUE,
		TAG_DONE
	);
}

static int fb_hArosExecute( const char *program, char **argv, int argument_count )
{
	struct CommandLineInterface *cli;
	struct fb_aros_child_result child_result;
	struct Process *child;
	BPTR child_input;
	BPTR error_output;
	BPTR segment;
	char *arguments;
	IPTR child_id;
	ULONG stack_size = AROS_STACKSIZE;

	/*
	 * Keep launch failures on the command's redirected output stream.  AROS
	 * shells do not always attach pr_CES to serial or file redirections, which
	 * otherwise hides the only actionable DOS error from build logs.
	 */
	error_output = Output();
	segment = LoadSeg( (CONST_STRPTR)program );
	if( segment == BNULL ) {
		FPrintf(
			error_output,
			(CONST_STRPTR)"FreeBASIC: unable to load '%s' (DOS error %ld)\n",
			program,
			IoErr()
		);
		return -1;
	}

	cli = Cli();
	if( cli != NULL && cli->cli_DefaultStack > 0 )
		stack_size = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;

	arguments = fb_hArosArguments( argv, argument_count );
	if( arguments == NULL ) {
		FPrintf(
			error_output,
			(CONST_STRPTR)"FreeBASIC: unable to allocate child arguments\n"
		);
		UnLoadSeg( segment );
		return -1;
	}
	child_result.result = RETURN_FAIL;
	child_input = Input();
	child = fb_hArosCreateChild(
		segment, arguments, child_input, Output(), error_output,
		stack_size, program, &child_result
	);
	if( child == NULL && child_input != BNULL ) {
		/*
		 * AROS cannot share every CLI input handle with an independent m68k
		 * process.  Compiler stages consume named files, so retry without stdin.
		 * Both launches deliberately avoid NP_CopyVars because m68k AROS can
		 * corrupt the allocator while freeLocalVars tears down the copied list.
		 */
		child = fb_hArosCreateChild(
			segment, arguments, BNULL, Output(), error_output,
			stack_size, program, &child_result
		);
	}
	free( arguments );
	if( child == NULL ) {
		FPrintf(
			error_output,
			(CONST_STRPTR)"FreeBASIC: unable to create process for '%s' (DOS error %ld)\n",
			program,
			IoErr()
		);
		UnLoadSeg( segment );
		return -1;
	}

	child_id = GetETask( (struct Task *)child )->et_UniqueID;
	ChildWait( child_id );
	UnLoadSeg( segment );
	ChildFree( child_id );
	return child_result.result;
}

FBCALL int fb_ExecEx( FBSTRING *program, FBSTRING *args, int do_fork )
{
	char buffer[MAX_PATH];
	char empty_arguments[] = "";
	char *arguments;
	char **argv;
	char *cursor;
	int argument_count = 0;
	int index;
	int result = -1;
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

	if( do_fork )
		result = fb_hArosExecute( buffer, argv, argument_count );
	else
		result = execvp( buffer, argv );

	FB_LOCK();
	fb_hInitConsole();
	FB_UNLOCK();

	return result;
}

/* end of aros/m68k/sys_execex.c */
