/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_isredir.c
 * Purpose: Provide io isredir runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleIsRedirected( int is_input )
{
	return (isatty( fileno( (is_input ? stdin : stdout) ) ) == 0) ? FB_TRUE : FB_FALSE;
}

/* end of drivers/unix/io_isredir.c */
