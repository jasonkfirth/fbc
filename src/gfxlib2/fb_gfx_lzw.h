/*
 * Project: FreeBASIC runtime libraries
 * File: fb_gfx_lzw.h
 * Purpose: Provide the legacy compact LZW codec.
 * Responsibilities: Compression tables and packed-code input/output helpers.
 * This file intentionally does NOT contain: Image file dispatch or platform presentation.
 */
/* tiny LZW codec,
   Based on code by Mark Nelson, Dr. Dobb's Journal October, 1989 */

#ifndef FB_GFX_LZW_H
#define FB_GFX_LZW_H


#define MAX_CODE	4095
#define TABLE_SIZE	5021

#define OUTPUT_CODE(c)						\
do {										\
	if (bit) {								\
		*out_buffer++ |= (unsigned char)((c & 0xF) << 4);	\
		*out_buffer++ = (unsigned char)(c >> 4);				\
		size++;								\
	}										\
	else {									\
		*out_buffer++ = (unsigned char)(c & 0xFF);			\
		*out_buffer = (unsigned char)(c >> 8);				\
	}										\
	size++;									\
	if (size >= *out_size)					\
		return -1;							\
	bit ^= 1;								\
} while (0)

#define INPUT_CODE(c)						\
do {										\
	if (bit) {								\
		c = (unsigned short)(*in_buffer++ >> 4);				\
		c |= (unsigned short)(*in_buffer++ << 4);				\
		in_size--;							\
	}										\
	else {									\
		c = (unsigned short)*in_buffer++;					\
		c |= (unsigned short)((*in_buffer & 0xF) << 8);		\
	}										\
	in_size--;								\
	bit ^= 1;								\
} while (0)


/* LZW table entries keep a fixed six-byte serialized stride. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#endif
typedef struct LZW_ENTRY
{
	short code;
	unsigned short prefix;
	unsigned char value;
	/* Keep the existing six-byte table-entry stride explicit. */
	unsigned char reserved_tail_padding;
} LZW_ENTRY;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

extern LZW_ENTRY fb_lzw_entry[TABLE_SIZE];

#endif

/* end of fb_gfx_lzw.h */
