/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_beep.c
 * Purpose: beep function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL void fb_Beep( void )
{
	putc('\a', stdout);
}

/* end of drivers/dos/sys_beep.c */
