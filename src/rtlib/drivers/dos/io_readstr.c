/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	return fgets( buffer, len, stdin );
}

/* end of drivers/dos/io_readstr.c */
