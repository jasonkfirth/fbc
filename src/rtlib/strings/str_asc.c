/*
 * Project: FreeBASIC runtime library
 * File: strings/str_asc.c
 * Purpose: asc function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL unsigned int fb_ASC( FBSTRING *str, ssize_t pos )
{
    unsigned int a;
	ssize_t len;

	if( str == NULL )
		return 0;

	len = FB_STRSIZE( str );
	if( (str->data == NULL) || (len == 0) || (pos <= 0) || (pos > len) )
		a = 0;
	else
		a = (unsigned char)str->data[pos-1];

	/* del if temp */
	fb_hStrDelTemp( str );

	return a;
}

/* end of strings/str_asc.c */
