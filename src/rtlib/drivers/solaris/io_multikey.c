/*
 * Project: FreeBASIC runtime library
 * File: drivers/solaris/io_multikey.c
 * Purpose: console multikey().
 * Responsibilities: Target-specific runtime support for solaris.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleMultikey( int scancode )
{
	fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return FB_FALSE;
}

/* end of drivers/solaris/io_multikey.c */
