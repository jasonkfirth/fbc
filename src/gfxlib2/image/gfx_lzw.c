/*
 * FreeBASIC gfxlib2: gfx_lzw.c
 * Purpose: Decode the legacy graphics LZW stream.
 * Responsibilities: image storage, conversion and checked file operations.
 * This file intentionally does NOT contain native windows or input dispatch.
 */

/* tiny LZW codec
 * Based on code by Mark Nelson, Dr. Dobb's Journal October, 1989
 */

#include <stddef.h>

#include "../fb_gfx.h"
#include "../fb_gfx_lzw.h"

LZW_ENTRY fb_lzw_entry[TABLE_SIZE];

static unsigned char *decode_string(unsigned char *buffer, int code)
{
	int index = 0;

	while (code > 255) {
		if (code >= MAX_CODE)
			return NULL;
		*buffer++ = fb_lzw_entry[code].value;
		code = fb_lzw_entry[code].prefix;
		if (index++ >= MAX_CODE - 1)
			return NULL;
	}
	*buffer = (unsigned char)code;
	return buffer;
}

static int read_input_code(const unsigned char **input, size_t *remaining,
	unsigned char *bit, unsigned short *code)
{
	const unsigned char *p;
	unsigned int value;

	if (!input || !*input || !remaining || !bit || !code || (*remaining < 2u))
		return -1;

	p = *input;
	if (*bit) {
		value = ((unsigned int)p[0] >> 4) | ((unsigned int)p[1] << 4);
		*input += 2;
		*remaining -= 2u;
	} else {
		value = (unsigned int)p[0] | (((unsigned int)p[1] & 0x0Fu) << 8);
		*input += 1;
		*remaining -= 1u;
	}
	*code = (unsigned short)value;
	*bit = (unsigned char)(*bit ^ 1u);
	return 0;
}

FBCALL int fb_hDecode
	(
		const unsigned char *in_buffer,
		ssize_t input_size,
		unsigned char *out_buffer,
		ssize_t *out_size
	)
{
	unsigned short new_code, old_code, next_code = 256;
	unsigned char decode_stack[MAX_CODE], *string, byte, bit = 0;
	size_t input_remaining;
	size_t output_capacity;
	size_t output_count;

	if (!in_buffer || (input_size <= 0) || !out_buffer || !out_size ||
	    (*out_size <= 0))
		return -1;
	input_remaining = (size_t)input_size;
	output_capacity = (size_t)*out_size;

	/* The shared codec table belongs to graphics, including off-screen calls.
	 * BLOAD may already hold the graphics lock; taking FB_LOCK here would
	 * reverse the runtime-hook lock order.
	 */
	FB_GRAPHICS_LOCK( );

	if (read_input_code(&in_buffer, &input_remaining, &bit, &old_code) != 0) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}
	/* The initial LZW code is a literal; the table has no entries yet. */
	if (old_code > 255) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}
	byte = (unsigned char)old_code;
	if (output_capacity == 0u) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}
	*out_buffer++ = (unsigned char)old_code;
	output_count = 1u;
	*out_size = 1;
	while (input_remaining > 0u) {
		if (read_input_code(&in_buffer, &input_remaining, &bit, &new_code) != 0) {
			FB_GRAPHICS_UNLOCK( );
			return -1;
		}
		if (new_code == MAX_CODE) {
			FB_GRAPHICS_UNLOCK( );
			return 0;
		}
		if (new_code >= next_code) {
			*decode_stack = byte;
			string = decode_string(decode_stack + 1, old_code);
		} else {
			string = decode_string(decode_stack, new_code);
		}
		if (!string) {
			FB_GRAPHICS_UNLOCK( );
			return -1;
		}
		byte = *string;
		while (string >= decode_stack) {
			if (output_count >= output_capacity) {
				FB_GRAPHICS_UNLOCK( );
				return -1;
			}
			*out_buffer++ = *string--;
			output_count++;
			*out_size = (ssize_t)output_count;
		}
		if (next_code < MAX_CODE) {
			fb_lzw_entry[next_code].prefix = old_code;
			fb_lzw_entry[next_code].value = byte;
			next_code++;
		}
		old_code = new_code;
	}

	FB_GRAPHICS_UNLOCK( );
	return -1;
}

/* end of gfx_lzw.c */
