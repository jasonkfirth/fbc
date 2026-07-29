/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_data.c

    Purpose:

        Decode FreeBASIC's established built-in font data for gfxlib3 without
        linking any gfxlib2 runtime object.

    Responsibilities:

        - decode the generated 12-bit LZW data with checked input bounds
        - initialize the immutable data once across calling threads
		- return the standard 8 by 8, 8 by 14, and 8 by 16 glyph tables

    This file intentionally does NOT contain:

        - glyph drawing or string layout
        - gfxlib2 framebuffer state
        - font selection policy
*/

#include "gfx3_data.h"

#include <stdatomic.h>

/*
    The generated file is an asset shared with the established graphics
    library, not a gfxlib2 implementation object. Keeping the canonical byte
    stream avoids allowing the two libraries' built-in glyphs to drift.
*/
#include "../gfxlib2/gfxdata_inline.h"

enum {
	FB_GFX3_LZW_END_CODE = 4095,
	FB_GFX3_LZW_TABLE_SIZE = 4096,
	FB_GFX3_DATA_UNINITIALIZED = 0,
	FB_GFX3_DATA_INITIALIZING = 1,
	FB_GFX3_DATA_READY = 2,
	FB_GFX3_DATA_FAILED = -1
};

typedef struct FB_GFX3_LZW_ENTRY {
	uint16_t prefix;
	uint8_t value;
} FB_GFX3_LZW_ENTRY;

static unsigned char decoded_data[DATA_SIZE];
static _Atomic int data_status;

/* ------------------------------------------------------------------------- */
/* Checked 12-bit LZW decoder                                                */
/* ------------------------------------------------------------------------- */

static int data_read_code(const unsigned char **input, size_t *input_size,
	int *high_nibble, uint16_t *code)
{
	const unsigned char *bytes;

	if ((input == NULL) || (*input == NULL) || (input_size == NULL) ||
	    (high_nibble == NULL) || (code == NULL) || (*input_size < 2u))
		return FB_GFX3_INVALID;
	bytes = *input;
	if (*high_nibble) {
		*code = (uint16_t)(((uint16_t)(bytes[0] >> 4)) |
			((uint16_t)bytes[1] << 4));
		*input += 2;
		*input_size -= 2u;
	} else {
		*code = (uint16_t)(((uint16_t)bytes[0]) |
			((uint16_t)(bytes[1] & 0x0Fu) << 8));
		*input += 1;
		*input_size -= 1u;
	}
	*high_nibble = !*high_nibble;
	return FB_GFX3_OK;
}

static int data_decode_string(const FB_GFX3_LZW_ENTRY *table,
	uint16_t code, unsigned char *stack, size_t *length)
{
	size_t count = 0;

	if ((table == NULL) || (stack == NULL) || (length == NULL))
		return FB_GFX3_INVALID;
	while (code > 255u) {
		if ((code >= FB_GFX3_LZW_END_CODE) ||
		    (count >= FB_GFX3_LZW_TABLE_SIZE - 1u))
			return FB_GFX3_FAILED;
		stack[count++] = table[code].value;
		code = table[code].prefix;
	}
	stack[count++] = (unsigned char)code;
	*length = count;
	return FB_GFX3_OK;
}

static int data_decode(void)
{
	FB_GFX3_LZW_ENTRY table[FB_GFX3_LZW_TABLE_SIZE];
	unsigned char stack[FB_GFX3_LZW_TABLE_SIZE];
	const unsigned char *input = compressed_data;
	size_t input_size = sizeof(compressed_data);
	size_t output_size = 0;
	size_t stack_size;
	uint16_t old_code;
	uint16_t new_code;
	uint16_t next_code = 256;
	uint8_t first_byte;
	int high_nibble = FALSE;
	int result;

	memset(table, 0, sizeof(table));
	result = data_read_code(&input, &input_size, &high_nibble, &old_code);
	if ((result != FB_GFX3_OK) || (old_code > 255u))
		return FB_GFX3_FAILED;
	decoded_data[output_size++] = (unsigned char)old_code;
	first_byte = (uint8_t)old_code;
	while (input_size > 0) {
		result = data_read_code(&input, &input_size, &high_nibble,
			&new_code);
		if (result != FB_GFX3_OK)
			return result;
		if (new_code == FB_GFX3_LZW_END_CODE)
			return (output_size == sizeof(decoded_data)) ?
				FB_GFX3_OK : FB_GFX3_FAILED;
		if (new_code >= next_code) {
			stack[0] = first_byte;
			result = data_decode_string(table, old_code, stack + 1,
				&stack_size);
			if (result == FB_GFX3_OK)
				stack_size++;
		} else {
			result = data_decode_string(table, new_code, stack,
				&stack_size);
		}
		if (result != FB_GFX3_OK)
			return result;
		first_byte = stack[stack_size - 1u];
		while (stack_size > 0) {
			if (output_size >= sizeof(decoded_data))
				return FB_GFX3_FAILED;
			decoded_data[output_size++] = stack[--stack_size];
		}
		if (next_code < FB_GFX3_LZW_END_CODE) {
			table[next_code].prefix = old_code;
			table[next_code].value = first_byte;
			next_code++;
		}
		old_code = new_code;
	}
	return FB_GFX3_FAILED;
}

/* ------------------------------------------------------------------------- */
/* Public immutable data access                                              */
/* ------------------------------------------------------------------------- */

static int data_ensure_ready(void)
{
	int expected = FB_GFX3_DATA_UNINITIALIZED;
	int status;

	if (atomic_compare_exchange_strong_explicit(&data_status, &expected,
	    FB_GFX3_DATA_INITIALIZING, memory_order_acq_rel,
	    memory_order_acquire)) {
		status = (data_decode() == FB_GFX3_OK) ?
			FB_GFX3_DATA_READY : FB_GFX3_DATA_FAILED;
		atomic_store_explicit(&data_status, status, memory_order_release);
		return (status == FB_GFX3_DATA_READY);
	}
	do {
		status = atomic_load_explicit(&data_status, memory_order_acquire);
	} while (status == FB_GFX3_DATA_INITIALIZING);
	return (status == FB_GFX3_DATA_READY);
}

const unsigned char *fb_gfx3_data_font_8x8(void)
{
	return data_ensure_ready() ? decoded_data + DATA_FONT_8 : NULL;
}

const unsigned char *fb_gfx3_data_font_8x14(void)
{
	return data_ensure_ready() ? decoded_data + DATA_FONT_14 : NULL;
}

const unsigned char *fb_gfx3_data_font_8x16(void)
{
	return data_ensure_ready() ? decoded_data + DATA_FONT_16 : NULL;
}

const unsigned char *fb_gfx3_data_palette_256(void)
{
	return data_ensure_ready() ? decoded_data + DATA_PAL_256 : NULL;
}

/* end of gfx3_data.c */
