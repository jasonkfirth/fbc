/*
 * Project: FreeBASIC runtime library
 * File: strings/str_bin_ptr.c
 * Purpose: bin(any ptr) function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_BIN_p( const void *p )
{
#ifdef HOST_64BIT
	return fb_BIN_l( (unsigned long long int)p );
#else
	return fb_BIN_i( (unsigned int)p );
#endif
}

FBCALL FBSTRING *fb_BINEx_p( const void *p, int digits )
{
#ifdef HOST_64BIT
	return fb_BINEx_l( (unsigned long long int)p, digits );
#else
	return fb_BINEx_i( (unsigned int)p, digits );
#endif
}

/* end of strings/str_bin_ptr.c */
