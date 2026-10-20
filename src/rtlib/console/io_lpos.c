/*
 * Project: FreeBASIC runtime library
 * File: console/io_lpos.c
 * Purpose: Returns the printers X position.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_LPos( int printer_index )
{
    int cur;
    char buffer[32];
	
    FB_LOCK();

    sprintf(buffer, "LPT%d:", (printer_index+1));
    cur = fb_DevPrinterGetOffset( buffer );

	FB_UNLOCK();
	
    return cur;
}

/* end of console/io_lpos.c */
