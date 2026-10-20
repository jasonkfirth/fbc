/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_oct_ptr.c
 * Purpose: woct(any ptr) function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrOct_p( const void *p )
{
#ifdef HOST_64BIT
	return fb_WstrOct_l( (unsigned long long int)p );
#else
	return fb_WstrOct_i( (unsigned int)p );
#endif
}

FBCALL FB_WCHAR *fb_WstrOctEx_p( const void *p, int digits )
{
#ifdef HOST_64BIT
	return fb_WstrOctEx_l( (unsigned long long int)p, digits );
#else
	return fb_WstrOctEx_i( (unsigned int)p, digits );
#endif
}

/* end of strings/wide/strw_oct_ptr.c */
