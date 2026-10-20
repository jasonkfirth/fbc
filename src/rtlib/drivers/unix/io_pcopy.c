/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_pcopy.c
 * Purpose: console pcopy function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsolePageCopy( int src, int dst )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of drivers/unix/io_pcopy.c */
