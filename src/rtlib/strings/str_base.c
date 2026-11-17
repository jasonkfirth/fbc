/*
 * Project: FreeBASIC runtime library
 * File: strings/str_base.c
 * Purpose: Provide str base runtime support.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL FBSTRING_ACCESS *fb_HEX ( int num )
{
	return fb_HEX_i ( (unsigned int)num );
}


/*:::::*/
FBCALL FBSTRING_ACCESS *fb_OCT ( int num )
{
	return fb_OCT_i ( (unsigned int)num );
}


/*:::::*/
FBCALL FBSTRING_ACCESS *fb_BIN ( int num )
{
	return fb_BIN_i ( (unsigned int)num );
}

/* end of strings/str_base.c */
