/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_execex.c
 * Purpose: Provide sys execex runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <limits.h>
#include <process.h>
#include <stdint.h>
#include <stdlib.h>

FBCALL int fb_ExecEx( FBSTRING_ACCESS *program, FBSTRING_ACCESS *args, int do_fork )
{
	char buffer[MAX_PATH], *arguments = NULL, **argv = NULL, *p;
	char *arguments_end;
	int i, argc = 0, res = -1;
	ssize_t len_arguments = 0;

	if( (program == NULL) || (program->data == NULL) ) 
	{
		fb_hStrDelTemp( args );
		fb_hStrDelTemp( program );
		return -1;
	}

	fb_hGetShortPath( program->data, buffer, MAX_PATH );

	if( args != NULL ) {
		len_arguments = FB_STRSIZE( args );
		if( (len_arguments < 0) ||
		    ((len_arguments > 0) && (args->data == NULL)) ||
		    ((size_t)len_arguments > SIZE_MAX - 1) ) {
			argc = -1;
		} else if( len_arguments > 0 ) {
			arguments = malloc( (size_t)len_arguments + 1 );
			if( arguments != NULL ) {
				arguments[(size_t)len_arguments] = 0;
				argc = fb_hParseArgs( arguments, args->data, len_arguments );
			} else {
				argc = -1;
			}
		}
	}

	FB_STRLOCK();

	fb_hStrDelTemp_NoLock( args );
	fb_hStrDelTemp_NoLock( program );

	FB_STRUNLOCK();

	if( argc < 0 || argc == INT_MAX ||
	    (size_t)argc > SIZE_MAX / sizeof( *argv ) - 2 )
		goto done;

	argc++; 			/* add 1 for program name */

	argv = malloc( sizeof( *argv ) * ((size_t)argc + 1) );
	if( argv == NULL )
		goto done;

	argv[0] = buffer;

	/* scan the processed args and set pointers */
	p = arguments;
	arguments_end = arguments ? arguments + (size_t)len_arguments + 1 : NULL;
	for( i=1 ; i<argc; i++)
	{
		if( (p == NULL) || (p >= arguments_end) )
			goto done;
		argv[i] = p;	/* set pointer to current argument */
		while( (p < arguments_end) && *p )
			p++;
		if( p >= arguments_end )
			goto done;
		p++;			/* skip to 1 char past next null char */
	}
	argv[argc] = NULL;

	/* NOTE: DJGPP info on 3rd arg of spawnv* functions is inconsistent;
	   in docs, defined as const char **;
	   in process.h, defined as char *const _argv[]
	*/

	if( do_fork )
		res = spawnv( P_WAIT, buffer, (char * const *)argv );
	else
		res = execv( buffer, (char * const *)argv );

	done:
	free( argv );
	free( arguments );
	return res;
}

/* end of drivers/dos/sys_execex.c */
