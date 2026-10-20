/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_gety.c
 * Purpose: Provide io gety runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetY( void )
{
	int res = EM_ASM_INT(
        return __fb_rtlib.console.pos_get();
    ,NULL);

    return 1+((res >> 8) & 0xFF);
}

/* end of drivers/js/io_gety.c */
