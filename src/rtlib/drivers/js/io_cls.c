/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_cls.c
 * Purpose: console CLS statement.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleClear( int mode )
{
	EM_ASM({
        __fb_rtlib.console.clear();
    });
}

/* end of drivers/js/io_cls.c */
