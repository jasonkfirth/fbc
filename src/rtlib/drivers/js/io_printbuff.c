/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_printbuff.c
 * Purpose: low-level print to console function stub.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
    EM_ASM_ARGS({
        __fb_rtlib.console.writeSubs(UTF8ToString($0), $1);
    }, buffer, len );
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	fb_ConsolePrintBufferEx( buffer, strlen(buffer), mask );
}

/* end of drivers/js/io_printbuff.c */
