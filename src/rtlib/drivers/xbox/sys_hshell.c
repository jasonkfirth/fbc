/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_hshell.c
 * Purpose: SHELL command.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

int fb_hShell( char *program )
{
	HANDLE handle;
	/* XLaunchXBE reboots the title without checking that the named file
	   exists. A failed SHELL/EXEC must return to BASIC instead of handing
	   a nonexistent image to firmware (or an emulator's launch dialog). */
	if( program == NULL || program[0] == '\0' || strlen( program ) >= MAX_PATH )
		return -1;
	handle = fb_hOpenFileForQuery( program );
	if( handle == INVALID_HANDLE_VALUE )
		return -1;
	CloseHandle( handle );
	XLaunchXBE( program );
	return -1;
}

/* end of drivers/xbox/sys_hshell.c */
