/*
 * Project: FreeBASIC runtime library
 * File: strings/str_size.h
 * Purpose: Check signed string capacities before reserving native storage.
 * Responsibilities: Alignment/growth limits and checked concatenation lengths.
 * This file intentionally does NOT contain: allocation, copying or descriptor ownership.
 */
#ifndef FB_STR_SIZE_H
#define FB_STR_SIZE_H

/* Descriptor capacities are signed. Reserve the terminating byte within that
   same range, and keep the usual 32-byte rounding and 12.5 percent growth when
   they fit. Near the limit the requested capacity remains valid without them. */
static __inline__ int fb_hStrCapacity( ssize_t length, int grow, ssize_t *capacity )
{
	size_t size, extra;
	const size_t limit = (size_t)FB_STRSIZEMSK - 1;

	if( length < 0 || (size_t)length > limit )
		return FALSE;
	size = (size_t)length;
	if( limit - size >= 31 )
		size = (size + 31) & ~(size_t)31;
	if( grow ) {
		extra = size >> 3;
		if( extra <= limit - size )
			size += extra;
	}
	*capacity = (ssize_t)size;
	return TRUE;
}

static __inline__ int fb_hStrAddSize( ssize_t first, ssize_t second, ssize_t *total )
{
	if( first < 0 || second < 0 || first > FB_STRSIZEMSK - 1 ||
	    second > (FB_STRSIZEMSK - 1) - first )
		return FALSE;
	*total = first + second;
	return TRUE;
}

#endif

/* end of strings/str_size.h */
