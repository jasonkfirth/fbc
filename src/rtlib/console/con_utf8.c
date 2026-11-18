/*
 * Project: FreeBASIC runtime library
 * File: console/con_utf8.c
 * Purpose: Decode a byte console record into a caller's WSTRING.
 * Responsibilities: Honor byte and wide bounds and preserve scalar boundaries.
 * This file intentionally does NOT contain: Input or device dispatch.
 */
#include "fb.h"

void fb_hConsoleBytesToWstr( FB_WCHAR *dst, ssize_t dst_chars, const char *src, ssize_t bytes )
{
	ssize_t capacity;
	ssize_t offset = 0, written = 0;

	if( dst == NULL || dst_chars < 0 )
		return;
	if( src == NULL || bytes <= 0 ) {
		dst[0] = 0;
		return;
	}

	/* A zero length denotes an unbounded WSTRING pointer, as in WstrAssign.
	   A UTF-8 record never expands to more wide units than source bytes. */
	capacity = dst_chars == 0 ? bytes : dst_chars - 1;
#if defined(HOST_DOS) || defined(HOST_XBOX) || defined(HOST_WINCE)
	/* The legacy console drivers supply their native byte encoding. */
	(void)offset;
	(void)written;
	fb_wstr_ConvFromA( dst, capacity, src );
#else
	while( offset < bytes && written < capacity ) {
		unsigned int scalar = fb_hUtf8Decode( src, bytes, &offset );
		if( scalar == 0 )
			break;
		if( sizeof(FB_WCHAR) == 2 && scalar > 0xFFFF ) {
			if( capacity - written < 2 )
				break;
			scalar -= 0x10000;
			dst[written++] = (FB_WCHAR)(0xD800 + (scalar >> 10));
			dst[written++] = (FB_WCHAR)(0xDC00 + (scalar & 0x3FF));
		} else {
			if( sizeof(FB_WCHAR) == 1 && scalar > 0xFF )
				scalar = '?';
			dst[written++] = (FB_WCHAR)scalar;
		}
	}
	dst[written] = 0;
#endif
}

/* end of console/con_utf8.c */
