/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_bin.c
 * Purpose: binw$ routines.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrBin_b ( unsigned char num )
{
	return fb_WstrBinEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrBin_s ( unsigned short num )
{
	return fb_WstrBinEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrBin_i ( unsigned int num )
{
	return fb_WstrBinEx_l( num, 0 );
}

FBCALL FB_WCHAR *fb_WstrBinEx_b( unsigned char num, int digits )
{
	return fb_WstrBinEx_l( num, digits );
}

FBCALL FB_WCHAR *fb_WstrBinEx_s( unsigned short num, int digits )
{
	return fb_WstrBinEx_l( num, digits );
}

FBCALL FB_WCHAR *fb_WstrBinEx_i ( unsigned int num, int digits )
{
	return fb_WstrBinEx_l( num, digits );
}

/* end of strings/wide/strw_bin.c */
