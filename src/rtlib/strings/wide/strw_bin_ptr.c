/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_bin_ptr.c
 * Purpose: wbin(any ptr) function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrBin_p( const void *p )
{
#ifdef HOST_64BIT
	return fb_WstrBin_l( (unsigned long long int)p );
#else
	return fb_WstrBin_i( (unsigned int)p );
#endif
}

FBCALL FB_WCHAR *fb_WstrBinEx_p( const void *p, int digits )
{
#ifdef HOST_64BIT
	return fb_WstrBinEx_l( (unsigned long long int)p, digits );
#else
	return fb_WstrBinEx_i( (unsigned int)p, digits );
#endif
}

/* end of strings/wide/strw_bin_ptr.c */
