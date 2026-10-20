/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_isredir.c
 * Purpose: Provide io isredir runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

int fb_ConsoleIsRedirected( int is_input )
{
	return (isatty( fileno( (is_input ? stdin : stdout) ) ) == 0) ? FB_TRUE : FB_FALSE;
}

/* end of drivers/dos/io_isredir.c */
