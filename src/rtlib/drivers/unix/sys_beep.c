/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/sys_beep.c
 * Purpose: beep function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

FBCALL void fb_Beep( void )
{
	fb_hTermOut(SEQ_BEEP, 0, 0);
}

/* end of drivers/unix/sys_beep.c */
