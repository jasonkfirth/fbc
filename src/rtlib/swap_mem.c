/*
 * FreeBASIC runtime library
 * -------------------------
 *
 * File: swap_mem.c
 *
 * Purpose:
 *
 *     Exchange two non-overlapping memory regions.
 *
 * Responsibilities:
 *
 *     - use word transfers when both regions are naturally aligned
 *     - preserve byte-accurate behavior for unaligned regions
 *     - serialize the exchange through the runtime lock
 *
 * This file intentionally does NOT contain:
 *
 *     - string length or allocation handling
 *     - overlapping-region semantics
 */

#include "fb.h"

FBCALL void fb_MemSwap( unsigned char *dst, unsigned char *src, ssize_t bytes )
{
	ssize_t i;
	ssize_t words = 0;
	unsigned int ti;
	unsigned char tb;

	if( (dst == NULL) || (src == NULL) || (bytes <= 0) )
		return;

	FB_LOCK();
	
	/*
	 * Fixed WSTRING buffers may be aligned to two bytes rather than four.
	 * MIPS traps unaligned word accesses, so the word path is valid only when
	 * both addresses satisfy the alignment required by unsigned int.
	 */
	if( ((((uintptr_t)src) | ((uintptr_t)dst)) &
	     (sizeof(unsigned int) - 1)) == 0 ) {
		words = bytes / sizeof(unsigned int);
		for( i = 0; i < words; i++ )
		{
			ti = *(unsigned int *)src;
			*(unsigned int *)src = *(unsigned int *)dst;
			*(unsigned int *)dst = ti;

			src += sizeof(unsigned int);
			dst += sizeof(unsigned int);
		}
	}

	/* remainder, or the complete unaligned exchange */
	bytes -= words * sizeof(unsigned int);
	for( i = 0; i < bytes; i++ )
	{
		tb = *src;
		*src++ = *dst;
		*dst++ = tb;
	}
	
	FB_UNLOCK();
}

/* end of swap_mem.c */
