/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_hshell.c
 * Purpose: SHELL command.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hShell( char *program )
{
	return system( program );
}

/* end of drivers/dos/sys_hshell.c */
