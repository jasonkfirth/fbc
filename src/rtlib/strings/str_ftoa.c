/*
 * Project: FreeBASIC runtime library
 * File: strings/str_ftoa.c
 * Purpose: float to string, internal usage.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>

char *fb_hFloat2Str( double val, char *buffer, int digits, int mask )
{
	ssize_t len, maxlen;
	char *p;

	if( (digits < 0) || (digits > INT_MAX - 8) ) {
		buffer[0] = '\0';
		return NULL;
	}

	if( mask & FB_F2A_ADDBLANK )
		p = &buffer[1];
	else
		p = buffer;

	maxlen = (ssize_t)digits + 1 + 6 + 1;

	/* The caller reserves digits plus exponent space; a truncation is rejected below. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
	len = snprintf( p, (size_t)maxlen, "%.*g", digits, val );
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

	if( len <= 0 || len >= maxlen )
	{
		buffer[0] = '\0';
		return NULL;
	}

	if( len > 0 )
	{
		/* skip the dot at end if any */
		if( p[len-1] == '.' )
			p[len-1] = '\0';
	}

	/* */
	if( (mask & FB_F2A_ADDBLANK) > 0 )
	{
		if( p[0] != '-' )
		{
			buffer[0] = ' ';
			return &buffer[0];
		}
		else
			return p;
	}
	else
		return p;
}

/* end of strings/str_ftoa.c */
