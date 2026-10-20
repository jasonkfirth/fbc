/*
 * Project: FreeBASIC runtime library
 * File: console/con_pos.c
 * Purpose: implementation of pos(dummy), simply redirects to fb_GetX().
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_Pos( int dummy )
{
	return fb_GetX();
}

/* end of console/con_pos.c */
