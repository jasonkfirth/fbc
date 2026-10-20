/*
 * Project: FreeBASIC runtime library
 * File: memory/mem_copyclear.c
 * Purpose: LSET for non-strings.
 * Responsibilities: Raw memory operations and internal list storage.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_MemCopyClear
	(
		unsigned char *dst,
		size_t dstlen,
		unsigned char *src,
		size_t srclen
	)
{
	size_t bytes;

	if( (dst == NULL) || (src == NULL) || (dstlen == 0) )
		return;

	bytes = (dstlen <= srclen? dstlen: srclen);
	
	/* move */
	if( bytes > 0 )
		memcpy( dst, src, bytes );

	/* clear remainder */
	dstlen -= bytes;
	if( dstlen > 0 )
		memset( &dst[bytes], 0, dstlen );
}

/* end of memory/mem_copyclear.c */
