/*
 * Project: FreeBASIC runtime library
 * File: strings/encoding/utf_size.h
 * Purpose: Check signed UTF counts before allocating native byte storage.
 * Responsibilities: Byte-count conversion and eight-unit decoder growth.
 * This file intentionally does NOT contain: character decoding or output-buffer ownership.
 */
#ifndef FB_UTF_SIZE_H
#define FB_UTF_SIZE_H

/* Public conversion lengths are ssize_t. Keep allocated byte counts within
   that same range so neither multiplication nor a later result can wrap. */
static __inline__ int fb_hUTFBytes( ssize_t chars, size_t width, size_t *bytes )
{
	if( chars < 0 || width == 0 || (size_t)chars > (size_t)FB_STRSIZEMSK / width )
		return FALSE;
	*bytes = (size_t)chars * width;
	return TRUE;
}

/* Decoders reserve eight units at a time, reclaiming the one remaining
   unit when the next character needs a UTF16 surrogate pair. */
static __inline__ int fb_hUTFGrow( ssize_t *capacity, ssize_t remaining,
                                 size_t width, size_t *bytes )
{
	ssize_t used;
	size_t limit;

	if( width == 0 || remaining < 0 || *capacity < remaining )
		return FALSE;
	used = *capacity - remaining;
	limit = (size_t)FB_STRSIZEMSK / width;
	if( (size_t)used > limit || limit - (size_t)used < 8 )
		return FALSE;
	*capacity = used + 8;
	return fb_hUTFBytes( *capacity, width, bytes );
}

#endif

/* end of strings/encoding/utf_size.h */
