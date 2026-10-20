/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_hex_ptr.c
 * Purpose: whex(any ptr) function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrHex_p( const void *p )
{
#ifdef HOST_64BIT
	return fb_WstrHex_l( (unsigned long long int)p );
#else
	return fb_WstrHex_i( (unsigned int)p );
#endif
}

FBCALL FB_WCHAR *fb_WstrHexEx_p( const void *p, int digits )
{
#ifdef HOST_64BIT
	return fb_WstrHexEx_l( (unsigned long long int)p, digits );
#else
	return fb_WstrHexEx_i( (unsigned int)p, digits );
#endif
}

/* end of strings/wide/strw_hex_ptr.c */
