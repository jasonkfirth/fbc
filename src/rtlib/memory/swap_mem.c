/*
 * Project: FreeBASIC runtime library
 * -------------------------
 *
 * File: memory/swap_mem.c
 *
 * Purpose:
 *
 *     Exchange two non-overlapping memory regions.
 *
 * Responsibilities:
 *
 *     - use word-sized copies without requiring aligned regions
 *     - preserve byte-accurate behavior for the final remainder
 *     - serialize the exchange through the runtime lock
 *
 * This file intentionally does NOT contain: *
 *     - string length or allocation handling
 *     - overlapping-region semantics
 */

#include "fb.h"

FBCALL void fb_MemSwap( unsigned char *dst, unsigned char *src, ssize_t bytes )
{
	size_t words, remainder;
	unsigned int src_word, dst_word;
	unsigned char tb;

	if( (dst == NULL) || (src == NULL) || (bytes <= 0) )
		return;

	FB_LOCK();

	/* memcpy preserves byte order without requiring word alignment. */
	words = (size_t)bytes / sizeof( src_word );
	for( ; words > 0; --words )
	{
		memcpy( &src_word, src, sizeof( src_word ) );
		memcpy( &dst_word, dst, sizeof( dst_word ) );
		memcpy( src, &dst_word, sizeof( dst_word ) );
		memcpy( dst, &src_word, sizeof( src_word ) );

		src += sizeof( src_word );
		dst += sizeof( dst_word );
	}

	remainder = (size_t)bytes % sizeof( src_word );
	while( remainder > 0 )
	{
		tb = *src;
		*src++ = *dst;
		*dst++ = tb;
		--remainder;
	}

	FB_UNLOCK();
}

/* end of memory/swap_mem.c */
