/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_oct.c
 * Purpose: woct$ routines.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrOct_b ( unsigned char num )
{
	return fb_WstrOctEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrOct_s ( unsigned short num )
{
	return fb_WstrOctEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrOct_i ( unsigned int num )
{
	return fb_WstrOctEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrOctEx_b ( unsigned char num, int digits )
{
	return fb_WstrOctEx_l( num, digits );
}

FBCALL FB_WCHAR *fb_WstrOctEx_s ( unsigned short num, int digits )
{
	return fb_WstrOctEx_l( num, digits );
}

FBCALL FB_WCHAR *fb_WstrOctEx_i ( unsigned int num, int digits )
{
	return fb_WstrOctEx_l( num, digits );
}

/* end of strings/wide/strw_oct.c */
