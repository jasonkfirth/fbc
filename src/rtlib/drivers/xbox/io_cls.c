/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/io_cls.c
 * Purpose: console CLS statement.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <hal/debug.h>
#include <hal/video.h>

void fb_ConsoleClear( int mode )
{
	(void)mode;

	XVideoSetMode( 640, 480, 32, REFRESH_DEFAULT );
	debugClearScreen();
}

/* end of drivers/xbox/io_cls.c */
