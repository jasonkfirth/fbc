/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_execex.c
 * Purpose: Provide sys execex runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_ExecEx( FBSTRING_ACCESS *program, FBSTRING_ACCESS *args, int do_fork )
{
	if( program != NULL && program->data != NULL )
		fb_hShell( program->data );
	/* A successful Xbox launch replaces this title. On failure, preserve
	   EXEC/RUN's -1 contract and release compiler-owned temporary strings. */
	fb_hStrDelTemp( args );
	fb_hStrDelTemp( program );
	fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	return -1;
}

/* end of drivers/xbox/sys_execex.c */
