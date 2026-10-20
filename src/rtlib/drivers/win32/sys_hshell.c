/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_hshell.c
 * Purpose: SHELL command.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hShell( char *program )
{
	/* SHELL is specified as a command-backed runtime call. */
	// NOLINTNEXTLINE(bugprone-command-processor)
	return system( program );
}

/* end of drivers/win32/sys_hshell.c */
