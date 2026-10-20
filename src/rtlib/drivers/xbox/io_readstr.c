/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	return fgets( buffer, len, stdin );
}

/* end of drivers/xbox/io_readstr.c */
