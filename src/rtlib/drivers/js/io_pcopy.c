/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_pcopy.c
 * Purpose: console pcopy function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsolePageCopy( int src, int dst )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of drivers/js/io_pcopy.c */
