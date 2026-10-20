/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_getcwd.c
 * Purpose: get current dir.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	return GetCurrentDirectory( maxlen, dst );
}

/* end of drivers/win32/sys_getcwd.c */
