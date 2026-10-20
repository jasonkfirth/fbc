/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_delay.c
 * Purpose: Provide sys delay runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL void fb_Delay( int msecs )
{
	Sleep( msecs );
}

/* end of drivers/win32/sys_delay.c */
