/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_mkdir.c
 * Purpose: mkdir function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_MkDir( FBSTRING *path )
{
	int res;

	res = fb_hMakeDir( path->data );

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}

/* end of drivers/xbox/sys_mkdir.c */
