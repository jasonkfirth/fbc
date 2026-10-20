/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_delay.c
 * Purpose: Provide sys delay runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL void fb_Delay( int msecs )
{
	Sleep( msecs );
}

/* end of drivers/xbox/sys_delay.c */
