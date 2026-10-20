/*
 * Project: FreeBASIC runtime library
 * File: strings/str_base.c
 * Purpose: Provide str base runtime support.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL FBSTRING *fb_HEX ( int num )
{
	return fb_HEX_i ( num );
}


/*:::::*/
FBCALL FBSTRING *fb_OCT ( int num )
{
	return fb_OCT_i ( num );
}


/*:::::*/
FBCALL FBSTRING *fb_BIN ( int num )
{
	return fb_BIN_i ( num );
}

/* end of strings/str_base.c */
