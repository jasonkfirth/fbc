/*
 * Project: FreeBASIC runtime library
 * File: legacy/qb_inkey.c
 * Purpose: QB compatible INKEY.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL FBSTRING *fb_InkeyQB( void )
{
	FBSTRING *res = fb_Inkey();
	
	FB_LOCK();
	
	if( res && res->data 
		&& ( FB_STRSIZE(res) == 2 ) 
		&& ( res->data[0] == FB_EXT_CHAR ) )
	{
		res->data[0] = 0;
	}

	FB_UNLOCK();
	
	return res;
}

/* end of legacy/qb_inkey.c */
