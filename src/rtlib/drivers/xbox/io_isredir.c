/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_isredir.c
 * Purpose: Provide io isredir runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleIsRedirected( int is_input )
{
	return FB_TRUE;
}

/* end of drivers/xbox/io_isredir.c */
