/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_scroll.c
 * Purpose: console scrolling for when VIEW is used.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

void fb_ConsoleScroll( int nrows )
{
    /*
    	The JavaScript console target has no addressable text screen to scroll.
    	Keeping this as a no-op matches the rest of the JS console backend.
    */
    (void)nrows;
}

/* end of drivers/js/io_scroll.c */
