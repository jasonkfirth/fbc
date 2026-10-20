/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_input.c
 * Purpose: console input helpers.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_hConsoleInputBufferChanged( void )
{
	return fb_KeyHit();
}

/* end of drivers/unix/io_input.c */
