/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_isredir.c
 * Purpose: Provide io isredir runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleIsRedirected( int is_input )
{
	return FB_FALSE;
}

/* end of drivers/js/io_isredir.c */
