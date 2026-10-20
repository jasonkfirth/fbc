/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_chdir.c
 * Purpose: chdir function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_ChDir( FBSTRING *path )
{
	int res;

	res = fb_hChangeDir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}

/* end of drivers/xbox/sys_chdir.c */
