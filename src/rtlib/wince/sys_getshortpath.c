/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_getshortpath.c

    Purpose:

        Preserve the runtime short-path helper contract on Windows CE.

    Responsibilities:

        - copy paths into a bounded destination buffer
        - terminate every non-empty destination buffer

    This file intentionally does NOT contain:

        - desktop 8.3 path conversion
        - filesystem access
        - path encoding conversion
*/

#include "../fb.h"

char *fb_hGetShortPath( char *source, char *destination,
	                    ssize_t maximum_length )
{
	size_t copy_length;

	if( (source == NULL) || (destination == NULL) || (maximum_length <= 0) )
		return destination;

	copy_length = strlen( source );
	if( copy_length >= (size_t)maximum_length )
		copy_length = (size_t)maximum_length - 1;

	memcpy( destination, source, copy_length );
	destination[copy_length] = '\0';

	return destination;
}

/* end of wince/sys_getshortpath.c */
