/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_hshell.c
 * Purpose: SHELL command.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hShell( char *program )
{
	XLaunchXBE( program );
	return -1;
}

/* end of drivers/xbox/sys_hshell.c */
