/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_inkey.c
 * Purpose: console INKEY() function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

/* Caller is expected to hold FB_LOCK() */
FBSTRING *fb_ConsoleInkey( void )
{
	return 0;
}

/* Doing synchronization manually here because getkey() is blocking */
int fb_ConsoleGetkey( void )
{
	return 0;
}

/* Caller is expected to hold FB_LOCK() */
int fb_ConsoleKeyHit( void )
{
	return 0;
}

/* end of drivers/xbox/io_inkey.c */
