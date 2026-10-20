/*
 * Project: FreeBASIC runtime library
 * File: drivers/dragonfly/io_multikey.c
 * Purpose: console multikey().
 * Responsibilities: Target-specific runtime support for dragonfly.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsoleMultikey( int scancode )
{
	fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return FB_FALSE;
}

/* end of drivers/dragonfly/io_multikey.c */
