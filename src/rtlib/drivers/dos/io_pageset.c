/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_pageset.c
 * Purpose: console 'screen , pg, pg' function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <dpmi.h>

int fb_ConsolePageSet ( int active, int visible )
{
	int res = fb_console_context.active | (fb_console_context.visible << 8);

	if( active >= 0 )
	{
		fb_console_context.active = active;
	}

	if( visible >= 0 )
	{
		if( fb_console_context.visible != visible )
		{
        	__dpmi_regs regs;
        	regs.h.ah = 0x05;
        	regs.h.al = visible;
        	__dpmi_int(0x10, &regs);

			fb_console_context.visible = visible;
		}
	}

	return res;
}

/* end of drivers/dos/io_pageset.c */
