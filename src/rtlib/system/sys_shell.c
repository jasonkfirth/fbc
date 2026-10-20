/*
 * Project: FreeBASIC runtime library
 * File: system/sys_shell.c
 * Purpose: SHELL command.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_Shell( FBSTRING *program )
{
	int errcode = -1;

	if( program && program->data ) {
		errcode = fb_hShell( program->data );
	}

	/* del if temp */
	fb_hStrDelTemp( program );

	return errcode;
}

/* end of system/sys_shell.c */
