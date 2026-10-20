/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_pageset.c
 * Purpose: console 'screen , pg, pg' function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_ConsolePageSet( int active, int visible )
{
	return -1;
}

/* end of drivers/xbox/io_pageset.c */
