/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_beep.c
 * Purpose: beep function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL void fb_Beep( void )
{
	Beep( 1000, 250 );
}

/* end of drivers/win32/sys_beep.c */
