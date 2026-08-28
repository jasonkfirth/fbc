/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_png.c

    Purpose:

        Read and write Portable Network Graphics files for BLOAD and BSAVE
        without adding a system libpng or zlib dependency.

    Responsibilities:

        - validate PNG chunks and checksums
        - decode zlib/DEFLATE streams used by PNG files
        - encode portable PNG files with a compact fixed-Huffman compressor
        - apply and reverse PNG scanline filters
        - convert between PNG samples and gfxlib3 image formats

    This file intentionally does NOT contain:

        - BLOAD or BSAVE filename dispatch
        - public runtime entry points
        - JPEG, GIF, or BMP handling

    The codec uses only the C runtime already required by gfxlib3.  This is
    deliberate: supported gfxlib3 targets do not need a matching system
    libpng ABI or an additional runtime dependency.
*/

#include "gfx3_api_internal.h"
#include "gfx3_png.h"
#include "gfx3_image.h"

#include <limits.h>

/* ------------------------------------------------------------------------- */
/* PNG constants and checked arithmetic                                      */
/* ------------------------------------------------------------------------- */

#define PNG_COLOR_GRAYSCALE       0
#define PNG_COLOR_TRUECOLOR       2
#define PNG_COLOR_INDEXED         3
#define PNG_COLOR_GRAYSCALE_ALPHA 4
#define PNG_COLOR_TRUECOLOR_ALPHA 6

#define PNG_DEFLATE_MAX_BITS      15
#define PNG_DEFLATE_WINDOW        32768u
#define PNG_ENCODE_HASH_SIZE      4096u
#define PNG_IDAT_CHUNK_SIZE       (1024u * 1024u)
#define PNG_SIZE_MAX              ((size_t)-1)

static const unsigned char png_signature[8] = {
	137, 80, 78, 71, 13, 10, 26, 10
};

static int png_add_size(size_t left, size_t right, size_t *result)
{
	if (right > (PNG_SIZE_MAX - left))
		return FALSE;
	*result = left + right;
	return TRUE;
}

static int png_multiply_size(size_t left, size_t right, size_t *result)
{
	if ((left != 0) && (right > (PNG_SIZE_MAX / left)))
		return FALSE;
	*result = left * right;
	return TRUE;
}

static uint32_t png_read_be32(const unsigned char *data)
{
	return ((uint32_t)data[0] << 24) |
	       ((uint32_t)data[1] << 16) |
	       ((uint32_t)data[2] << 8) |
	       (uint32_t)data[3];
}

static void png_write_be32(unsigned char *data, uint32_t value)
{
	data[0] = (unsigned char)(value >> 24);
	data[1] = (unsigned char)(value >> 16);
	data[2] = (unsigned char)(value >> 8);
	data[3] = (unsigned char)value;
}

/* ------------------------------------------------------------------------- */
/* CRC-32 and Adler-32                                                        */
/* ------------------------------------------------------------------------- */

static uint32_t png_crc32_update(uint32_t crc, const unsigned char *data,
	size_t size)
{
	size_t i;
	unsigned int bit;

	for (i = 0; i < size; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			if (crc & 1u)
				crc = (crc >> 1) ^ 0xEDB88320u;
			else
				crc >>= 1;
		}
	}
	return crc;
}

static uint32_t png_adler32(const unsigned char *data, size_t size)
{
	uint32_t first = 1;
	uint32_t second = 0;
	size_t block;
	size_t i;

	/*
		Reducing every 5,552 bytes keeps both sums inside 32 bits.  This is
		the largest block size used by the reference Adler-32 algorithm.
	*/
	while (size != 0) {
		block = (size > 5552u) ? 5552u : size;
		size -= block;
		for (i = 0; i < block; i++) {
			first += *data++;
			second += first;
		}
		first %= 65521u;
		second %= 65521u;
	}
	return (second << 16) | first;
}

/* ------------------------------------------------------------------------- */
/* Growable byte and bit output                                              */
/* ------------------------------------------------------------------------- */

typedef struct PNG_BYTE_BUFFER {
	unsigned char *data;
	size_t size;
	size_t capacity;
	int failed;
} PNG_BYTE_BUFFER;

typedef struct PNG_BIT_WRITER {
	PNG_BYTE_BUFFER bytes;
	unsigned int current;
	unsigned int used_bits;
} PNG_BIT_WRITER;

static void png_byte_buffer_release(PNG_BYTE_BUFFER *buffer)
{
	free(buffer->data);
	buffer->data = NULL;
	buffer->size = 0;
	buffer->capacity = 0;
	buffer->failed = FALSE;
}

static int png_byte_buffer_reserve(PNG_BYTE_BUFFER *buffer, size_t extra)
{
	size_t required;
	size_t capacity;
	unsigned char *replacement;

	if (buffer->failed)
		return FALSE;
	if (!png_add_size(buffer->size, extra, &required)) {
		buffer->failed = TRUE;
		return FALSE;
	}
	if (required <= buffer->capacity)
		return TRUE;

	capacity = buffer->capacity;
	if (capacity < 256u)
		capacity = 256u;
	while (capacity < required) {
		if (capacity > (PNG_SIZE_MAX / 2u)) {
			capacity = required;
			break;
		}
		capacity *= 2u;
	}
	replacement = (unsigned char *)realloc(buffer->data, capacity);
	if (replacement == NULL) {
		buffer->failed = TRUE;
		return FALSE;
	}
	buffer->data = replacement;
	buffer->capacity = capacity;
	return TRUE;
}

static int png_byte_buffer_append(PNG_BYTE_BUFFER *buffer,
	const unsigned char *data, size_t size)
{
	if (!png_byte_buffer_reserve(buffer, size))
		return FALSE;
	if (size != 0)
		memcpy(buffer->data + buffer->size, data, size);
	buffer->size += size;
	return TRUE;
}

static int png_byte_buffer_put(PNG_BYTE_BUFFER *buffer, unsigned int value)
{
	unsigned char byte = (unsigned char)value;

	return png_byte_buffer_append(buffer, &byte, 1);
}

static int png_bit_writer_put(PNG_BIT_WRITER *writer, unsigned int value,
	unsigned int count)
{
	unsigned int bit;

	for (bit = 0; bit < count; bit++) {
		writer->current |= ((value >> bit) & 1u) << writer->used_bits;
		writer->used_bits++;
		if (writer->used_bits == 8u) {
			if (!png_byte_buffer_put(&writer->bytes, writer->current))
				return FALSE;
			writer->current = 0;
			writer->used_bits = 0;
		}
	}
	return TRUE;
}

static int png_bit_writer_align(PNG_BIT_WRITER *writer)
{
	if (writer->used_bits != 0) {
		if (!png_byte_buffer_put(&writer->bytes, writer->current))
			return FALSE;
		writer->current = 0;
		writer->used_bits = 0;
	}
	return TRUE;
}

static unsigned int png_reverse_bits(unsigned int value, unsigned int count)
{
	unsigned int reversed = 0;

	while (count-- != 0) {
		reversed = (reversed << 1) | (value & 1u);
		value >>= 1;
	}
	return reversed;
}

/* ------------------------------------------------------------------------- */
/* Fixed-Huffman DEFLATE encoder                                             */
/* ------------------------------------------------------------------------- */

static const unsigned short png_length_base[29] = {
	3, 4, 5, 6, 7, 8, 9, 10,
	11, 13, 15, 17,
	19, 23, 27, 31,
	35, 43, 51, 59,
	67, 83, 99, 115,
	131, 163, 195, 227, 258
};

static const unsigned char png_length_extra[29] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
	4, 4, 4, 4,
	5, 5, 5, 5, 0
};

static const unsigned short png_distance_base[30] = {
	1, 2, 3, 4,
	5, 7,
	9, 13,
	17, 25,
	33, 49,
	65, 97,
	129, 193,
	257, 385,
	513, 769,
	1025, 1537,
	2049, 3073,
	4097, 6145,
	8193, 12289,
	16385, 24577
};

static const unsigned char png_distance_extra[30] = {
	0, 0, 0, 0,
	1, 1,
	2, 2,
	3, 3,
	4, 4,
	5, 5,
	6, 6,
	7, 7,
	8, 8,
	9, 9,
	10, 10,
	11, 11,
	12, 12,
	13, 13
};

static int png_deflate_fixed_symbol(PNG_BIT_WRITER *writer,
	unsigned int symbol)
{
	unsigned int code;
	unsigned int bits;

	if (symbol <= 143u) {
		code = 0x30u + symbol;
		bits = 8;
	} else if (symbol <= 255u) {
		code = 0x190u + (symbol - 144u);
		bits = 9;
	} else if (symbol <= 279u) {
		code = symbol - 256u;
		bits = 7;
	} else if (symbol <= 287u) {
		code = 0xC0u + (symbol - 280u);
		bits = 8;
	} else {
		return FALSE;
	}

	return png_bit_writer_put(writer, png_reverse_bits(code, bits), bits);
}

static int png_deflate_length(PNG_BIT_WRITER *writer, size_t length)
{
	unsigned int code;
	unsigned int extra;

	for (code = 0; code < 29u; code++) {
		size_t maximum = png_length_base[code];
		if (png_length_extra[code] != 0)
			maximum += ((size_t)1u << png_length_extra[code]) - 1u;
		if (length <= maximum)
			break;
	}
	if (code == 29u)
		return FALSE;

	extra = png_length_extra[code];
	if (!png_deflate_fixed_symbol(writer, 257u + code))
		return FALSE;
	if (extra != 0) {
		if (!png_bit_writer_put(writer,
		    (unsigned int)(length - png_length_base[code]), extra))
			return FALSE;
	}
	return TRUE;
}

static int png_deflate_distance(PNG_BIT_WRITER *writer, size_t distance)
{
	unsigned int code;
	unsigned int extra;

	for (code = 0; code < 30u; code++) {
		size_t maximum = png_distance_base[code];
		if (png_distance_extra[code] != 0)
			maximum += ((size_t)1u << png_distance_extra[code]) - 1u;
		if (distance <= maximum)
			break;
	}
	if (code == 30u)
		return FALSE;

	if (!png_bit_writer_put(writer, png_reverse_bits(code, 5), 5))
		return FALSE;
	extra = png_distance_extra[code];
	if (extra != 0) {
		if (!png_bit_writer_put(writer,
		    (unsigned int)(distance - png_distance_base[code]), extra))
			return FALSE;
	}
	return TRUE;
}

static unsigned int png_deflate_hash(const unsigned char *data)
{
	uint32_t value;

	value = ((uint32_t)data[0] * 251u) ^
	        ((uint32_t)data[1] * 31u) ^
	        (uint32_t)data[2];
	return (unsigned int)(value & (PNG_ENCODE_HASH_SIZE - 1u));
}

static void png_deflate_insert(const unsigned char *data, size_t size,
	size_t position, uint32_t *heads, uint16_t *previous)
{
	unsigned int hash;
	uint32_t prior;
	size_t distance;

	if ((size - position) < 3u)
		return;
	hash = png_deflate_hash(data + position);
	prior = heads[hash];
	previous[position & (PNG_DEFLATE_WINDOW - 1u)] = 0;
	if ((prior != UINT32_MAX) && ((size_t)prior < position)) {
		distance = position - (size_t)prior;
		if (distance <= PNG_DEFLATE_WINDOW)
			previous[position & (PNG_DEFLATE_WINDOW - 1u)] =
				(uint16_t)distance;
	}
	heads[hash] = (uint32_t)position;
}

static void png_deflate_find_match(const unsigned char *data, size_t size,
	size_t position, const uint32_t *heads, const uint16_t *previous,
	size_t *best_length, size_t *best_distance)
{
	unsigned int hash;
	uint32_t candidate;
	unsigned int attempts = 0;
	size_t maximum;
	size_t length;
	size_t distance;
	uint16_t link;

	*best_length = 0;
	*best_distance = 0;
	if ((size - position) < 3u)
		return;

	hash = png_deflate_hash(data + position);
	candidate = heads[hash];
	maximum = size - position;
	if (maximum > 258u)
		maximum = 258u;

	while ((candidate != UINT32_MAX) && ((size_t)candidate < position) &&
	       (attempts++ < 64u)) {
		distance = position - (size_t)candidate;
		if (distance > PNG_DEFLATE_WINDOW)
			break;
		if ((data[candidate] == data[position]) &&
		    (data[candidate + 1u] == data[position + 1u]) &&
		    (data[candidate + 2u] == data[position + 2u])) {
			length = 3u;
			while ((length < maximum) &&
			       (data[(size_t)candidate + length] ==
			        data[position + length]))
				length++;
			if (length > *best_length) {
				*best_length = length;
				*best_distance = distance;
				if (length == maximum)
					break;
			}
		}

		link = previous[candidate & (PNG_DEFLATE_WINDOW - 1u)];
		if ((link == 0) || ((size_t)link > (size_t)candidate))
			break;
		candidate -= link;
	}
}

static int png_zlib_compress(const unsigned char *data, size_t size,
	unsigned char **output, size_t *output_size)
{
	PNG_BIT_WRITER writer;
	uint32_t *heads = NULL;
	uint16_t *previous = NULL;
	uint32_t checksum;
	unsigned char trailer[4];
	size_t position;
	size_t length;
	size_t distance;
	size_t i;
	int result = FB_GFX3_OK;

	memset(&writer, 0, sizeof(writer));
	*output = NULL;
	*output_size = 0;

	/*
		The match tables store absolute input positions in 32 bits.  PNG
		chunks use 32-bit lengths too, and rejecting larger single images
		keeps this encoder deterministic on both 32-bit and 64-bit targets.
	*/
	if (size > UINT32_MAX)
		return FB_GFX3_INVALID;

	heads = (uint32_t *)malloc(PNG_ENCODE_HASH_SIZE * sizeof(uint32_t));
	previous = (uint16_t *)calloc(PNG_DEFLATE_WINDOW, sizeof(uint16_t));
	if ((heads == NULL) || (previous == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}
	for (i = 0; i < PNG_ENCODE_HASH_SIZE; i++)
		heads[i] = UINT32_MAX;

	/* RFC 1950 header: DEFLATE, 32 KiB window, fastest compression. */
	if (!png_byte_buffer_put(&writer.bytes, 0x78) ||
	    !png_byte_buffer_put(&writer.bytes, 0x01)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}

	/* One final fixed-Huffman block. */
	if (!png_bit_writer_put(&writer, 3u, 3)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}

	position = 0;
	while (position < size) {
		png_deflate_find_match(data, size, position, heads, previous,
			&length, &distance);
		if (length >= 3u) {
			if (!png_deflate_length(&writer, length) ||
			    !png_deflate_distance(&writer, distance)) {
				result = FB_GFX3_OUT_OF_MEMORY;
				goto exit;
			}
			for (i = 0; i < length; i++)
				png_deflate_insert(data, size, position + i,
					heads, previous);
			position += length;
		} else {
			if (!png_deflate_fixed_symbol(&writer, data[position])) {
				result = FB_GFX3_OUT_OF_MEMORY;
				goto exit;
			}
			png_deflate_insert(data, size, position, heads, previous);
			position++;
		}
	}

	if (!png_deflate_fixed_symbol(&writer, 256u) ||
	    !png_bit_writer_align(&writer)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}

	checksum = png_adler32(data, size);
	png_write_be32(trailer, checksum);
	if (!png_byte_buffer_append(&writer.bytes, trailer, sizeof(trailer))) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}

	*output = writer.bytes.data;
	*output_size = writer.bytes.size;
	writer.bytes.data = NULL;
	writer.bytes.size = 0;
	writer.bytes.capacity = 0;

exit:
	free(previous);
	free(heads);
	png_byte_buffer_release(&writer.bytes);
	return result;
}

/* ------------------------------------------------------------------------- */
/* DEFLATE reader                                                            */
/* ------------------------------------------------------------------------- */

typedef struct PNG_BIT_READER {
	const unsigned char *data;
	size_t size;
	size_t position;
	unsigned int current;
	unsigned int remaining_bits;
	int failed;
} PNG_BIT_READER;

typedef struct PNG_HUFFMAN {
	unsigned short count[PNG_DEFLATE_MAX_BITS + 1];
	unsigned short symbol[288];
	unsigned int symbol_count;
} PNG_HUFFMAN;

static int png_bit_reader_get(PNG_BIT_READER *reader, unsigned int count,
	unsigned int *value)
{
	unsigned int result = 0;
	unsigned int bit;

	if (count > 24u) {
		reader->failed = TRUE;
		return FALSE;
	}
	for (bit = 0; bit < count; bit++) {
		if (reader->remaining_bits == 0) {
			if (reader->position >= reader->size) {
				reader->failed = TRUE;
				return FALSE;
			}
			reader->current = reader->data[reader->position++];
			reader->remaining_bits = 8;
		}
		result |= (reader->current & 1u) << bit;
		reader->current >>= 1;
		reader->remaining_bits--;
	}
	*value = result;
	return TRUE;
}

static void png_bit_reader_align(PNG_BIT_READER *reader)
{
	reader->current = 0;
	reader->remaining_bits = 0;
}

static int png_huffman_construct(PNG_HUFFMAN *table,
	const unsigned char *lengths, unsigned int count)
{
	unsigned short offsets[PNG_DEFLATE_MAX_BITS + 1];
	unsigned int symbol;
	unsigned int length;
	int available = 1;

	if (count > 288u)
		return FALSE;
	memset(table, 0, sizeof(*table));
	table->symbol_count = count;
	for (symbol = 0; symbol < count; symbol++) {
		if (lengths[symbol] > PNG_DEFLATE_MAX_BITS)
			return FALSE;
		table->count[lengths[symbol]]++;
	}

	/*
		Zero-length entries are absent symbols rather than codes.  A tree
		with no codes cannot decode anything and is always invalid here.
	*/
	if (table->count[0] == count)
		return FALSE;
	for (length = 1; length <= PNG_DEFLATE_MAX_BITS; length++) {
		available <<= 1;
		available -= table->count[length];
		if (available < 0)
			return FALSE;
	}

	offsets[1] = 0;
	for (length = 1; length < PNG_DEFLATE_MAX_BITS; length++)
		offsets[length + 1] = offsets[length] + table->count[length];
	for (symbol = 0; symbol < count; symbol++) {
		length = lengths[symbol];
		if (length != 0)
			table->symbol[offsets[length]++] = (unsigned short)symbol;
	}
	return TRUE;
}

static int png_huffman_decode(PNG_BIT_READER *reader,
	const PNG_HUFFMAN *table, unsigned int *symbol)
{
	unsigned int code = 0;
	unsigned int first = 0;
	unsigned int index = 0;
	unsigned int length;
	unsigned int bit;
	unsigned int count;

	for (length = 1; length <= PNG_DEFLATE_MAX_BITS; length++) {
		if (!png_bit_reader_get(reader, 1, &bit))
			return FALSE;
		code |= bit;
		count = table->count[length];
		if (code < (first + count)) {
			index += code - first;
			if (index >= table->symbol_count)
				return FALSE;
			*symbol = table->symbol[index];
			return TRUE;
		}
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	return FALSE;
}

static int png_deflate_fixed_tables(PNG_HUFFMAN *literal,
	PNG_HUFFMAN *distance)
{
	unsigned char lengths[288];
	unsigned char distances[32];
	unsigned int symbol;

	for (symbol = 0; symbol <= 143u; symbol++)
		lengths[symbol] = 8;
	for (; symbol <= 255u; symbol++)
		lengths[symbol] = 9;
	for (; symbol <= 279u; symbol++)
		lengths[symbol] = 7;
	for (; symbol <= 287u; symbol++)
		lengths[symbol] = 8;
	for (symbol = 0; symbol < 32u; symbol++)
		distances[symbol] = 5;

	return png_huffman_construct(literal, lengths, 288) &&
	       png_huffman_construct(distance, distances, 32);
}

static int png_deflate_dynamic_tables(PNG_BIT_READER *reader,
	PNG_HUFFMAN *literal, PNG_HUFFMAN *distance)
{
	static const unsigned char order[19] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};
	unsigned char code_lengths[19];
	unsigned char lengths[286 + 32];
	PNG_HUFFMAN code_table;
	unsigned int literal_count;
	unsigned int distance_count;
	unsigned int code_count;
	unsigned int total;
	unsigned int index;
	unsigned int symbol;
	unsigned int repeat;
	unsigned int extra;
	unsigned int value;

	if (!png_bit_reader_get(reader, 5, &value))
		return FALSE;
	literal_count = value + 257u;
	if (!png_bit_reader_get(reader, 5, &value))
		return FALSE;
	distance_count = value + 1u;
	if (!png_bit_reader_get(reader, 4, &value))
		return FALSE;
	code_count = value + 4u;
	if ((literal_count > 286u) || (distance_count > 32u))
		return FALSE;

	memset(code_lengths, 0, sizeof(code_lengths));
	for (index = 0; index < code_count; index++) {
		if (!png_bit_reader_get(reader, 3, &value))
			return FALSE;
		code_lengths[order[index]] = (unsigned char)value;
	}
	if (!png_huffman_construct(&code_table, code_lengths, 19))
		return FALSE;

	total = literal_count + distance_count;
	memset(lengths, 0, sizeof(lengths));
	index = 0;
	while (index < total) {
		if (!png_huffman_decode(reader, &code_table, &symbol))
			return FALSE;
		if (symbol <= 15u) {
			lengths[index++] = (unsigned char)symbol;
			continue;
		}

		if (symbol == 16u) {
			if ((index == 0) || !png_bit_reader_get(reader, 2, &extra))
				return FALSE;
			repeat = extra + 3u;
			value = lengths[index - 1u];
		} else if (symbol == 17u) {
			if (!png_bit_reader_get(reader, 3, &extra))
				return FALSE;
			repeat = extra + 3u;
			value = 0;
		} else if (symbol == 18u) {
			if (!png_bit_reader_get(reader, 7, &extra))
				return FALSE;
			repeat = extra + 11u;
			value = 0;
		} else {
			return FALSE;
		}

		if (repeat > (total - index))
			return FALSE;
		while (repeat-- != 0)
			lengths[index++] = (unsigned char)value;
	}

	/* Every compressed block requires the end-of-block code. */
	if (lengths[256] == 0)
		return FALSE;
	if (!png_huffman_construct(literal, lengths, literal_count))
		return FALSE;

	/*
		A dynamic literal-only block can have one distance entry whose length
		is zero.  Preserve that empty table so any unexpected length code is
		rejected instead of being paired with an invented distance symbol.
	*/
	if ((distance_count == 1u) && (lengths[literal_count] == 0)) {
		memset(distance, 0, sizeof(*distance));
		return TRUE;
	}
	return png_huffman_construct(distance, lengths + literal_count,
		distance_count);
}

static int png_deflate_compressed_block(PNG_BIT_READER *reader,
	unsigned char *output, size_t output_capacity, size_t *output_position,
	const PNG_HUFFMAN *literal, const PNG_HUFFMAN *distance)
{
	unsigned int symbol;
	unsigned int distance_symbol;
	unsigned int extra_value;
	unsigned int extra_bits;
	size_t length;
	size_t copy_distance;
	size_t index;

	for (;;) {
		if (!png_huffman_decode(reader, literal, &symbol))
			return FALSE;
		if (symbol < 256u) {
			if (*output_position >= output_capacity)
				return FALSE;
			output[(*output_position)++] = (unsigned char)symbol;
			continue;
		}
		if (symbol == 256u)
			return TRUE;
		if ((symbol < 257u) || (symbol > 285u))
			return FALSE;

		index = symbol - 257u;
		length = png_length_base[index];
		extra_bits = png_length_extra[index];
		if (extra_bits != 0) {
			if (!png_bit_reader_get(reader, extra_bits, &extra_value))
				return FALSE;
			length += extra_value;
		}

		if (!png_huffman_decode(reader, distance, &distance_symbol))
			return FALSE;
		if (distance_symbol >= 30u)
			return FALSE;
		copy_distance = png_distance_base[distance_symbol];
		extra_bits = png_distance_extra[distance_symbol];
		if (extra_bits != 0) {
			if (!png_bit_reader_get(reader, extra_bits, &extra_value))
				return FALSE;
			copy_distance += extra_value;
		}

		if ((copy_distance == 0) || (copy_distance > *output_position) ||
		    (length > (output_capacity - *output_position)))
			return FALSE;
		while (length-- != 0) {
			output[*output_position] =
				output[*output_position - copy_distance];
			(*output_position)++;
		}
	}
}

static int png_zlib_decompress(const unsigned char *data, size_t size,
	unsigned char *output, size_t output_size)
{
	PNG_BIT_READER reader;
	PNG_HUFFMAN literal;
	PNG_HUFFMAN distance;
	unsigned int final_block = 0;
	unsigned int block_type;
	unsigned int value;
	unsigned int inverse;
	size_t output_position = 0;
	size_t stored_size;
	uint32_t expected_adler;

	if (size < 6u)
		return FALSE;
	if ((data[0] & 0x0Fu) != 8u)
		return FALSE;
	if ((data[0] >> 4) > 7u)
		return FALSE;
	if ((((unsigned int)data[0] << 8) | data[1]) % 31u)
		return FALSE;
	if (data[1] & 0x20u)
		return FALSE;

	memset(&reader, 0, sizeof(reader));
	reader.data = data;
	reader.size = size;
	reader.position = 2;

	while (!final_block) {
		if (!png_bit_reader_get(&reader, 1, &final_block) ||
		    !png_bit_reader_get(&reader, 2, &block_type))
			return FALSE;

		if (block_type == 0u) {
			png_bit_reader_align(&reader);
			if ((reader.position + 4u) > reader.size)
				return FALSE;
			value = (unsigned int)reader.data[reader.position] |
			        ((unsigned int)reader.data[reader.position + 1u] << 8);
			inverse = (unsigned int)reader.data[reader.position + 2u] |
			          ((unsigned int)reader.data[reader.position + 3u] << 8);
			reader.position += 4u;
			if (((value ^ 0xFFFFu) & 0xFFFFu) != inverse)
				return FALSE;
			stored_size = value;
			if ((stored_size > (reader.size - reader.position)) ||
			    (stored_size > (output_size - output_position)))
				return FALSE;
			if (stored_size != 0)
				memcpy(output + output_position,
					reader.data + reader.position, stored_size);
			reader.position += stored_size;
			output_position += stored_size;
		} else if (block_type == 1u) {
			if (!png_deflate_fixed_tables(&literal, &distance) ||
			    !png_deflate_compressed_block(&reader, output,
			    output_size, &output_position, &literal, &distance))
				return FALSE;
		} else if (block_type == 2u) {
			if (!png_deflate_dynamic_tables(&reader, &literal, &distance) ||
			    !png_deflate_compressed_block(&reader, output,
			    output_size, &output_position, &literal, &distance))
				return FALSE;
		} else {
			return FALSE;
		}
	}

	png_bit_reader_align(&reader);
	if ((reader.position + 4u) != reader.size)
		return FALSE;
	expected_adler = png_read_be32(reader.data + reader.position);
	if (output_position != output_size)
		return FALSE;
	return expected_adler == png_adler32(output, output_size);
}

/* ------------------------------------------------------------------------- */
/* PNG chunk I/O                                                             */
/* ------------------------------------------------------------------------- */

static int png_write_chunk(FILE *file, const char type[4],
	const unsigned char *data, size_t size)
{
	unsigned char header[8];
	unsigned char checksum_bytes[4];
	uint32_t checksum;

	if (size > UINT32_MAX)
		return FALSE;
	png_write_be32(header, (uint32_t)size);
	header[4] = (unsigned char)type[0];
	header[5] = (unsigned char)type[1];
	header[6] = (unsigned char)type[2];
	header[7] = (unsigned char)type[3];
	checksum = png_crc32_update(0xFFFFFFFFu, header + 4, 4);
	checksum = png_crc32_update(checksum, data, size) ^ 0xFFFFFFFFu;
	png_write_be32(checksum_bytes, checksum);

	if (fwrite(header, sizeof(header), 1, file) != 1)
		return FALSE;
	if ((size != 0) && (fwrite(data, size, 1, file) != 1))
		return FALSE;
	return fwrite(checksum_bytes, sizeof(checksum_bytes), 1, file) == 1;
}

static int png_read_exact(FILE *file, unsigned char *data, size_t size)
{
	if (size == 0)
		return TRUE;
	return fread(data, size, 1, file) == 1;
}

static int png_skip_and_crc(FILE *file, size_t size, uint32_t *crc)
{
	unsigned char buffer[1024];
	size_t block;

	while (size != 0) {
		block = (size > sizeof(buffer)) ? sizeof(buffer) : size;
		if (!png_read_exact(file, buffer, block))
			return FALSE;
		*crc = png_crc32_update(*crc, buffer, block);
		size -= block;
	}
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Parsed PNG image metadata                                                 */
/* ------------------------------------------------------------------------- */

typedef struct PNG_IMAGE {
	uint32_t width;
	uint32_t height;
	unsigned int bit_depth;
	unsigned int color_type;
	unsigned int channels;
	unsigned int interlace;
	unsigned char palette[256 * 3];
	unsigned char palette_alpha[256];
	unsigned int palette_entries;
	unsigned int palette_alpha_entries;
	unsigned int transparent_gray;
	unsigned int transparent_red;
	unsigned int transparent_green;
	unsigned int transparent_blue;
	int has_transparent_gray;
	int has_transparent_color;
	PNG_BYTE_BUFFER compressed;
	unsigned char *filtered;
	size_t filtered_size;
} PNG_IMAGE;

static void png_image_release(PNG_IMAGE *image)
{
	png_byte_buffer_release(&image->compressed);
	free(image->filtered);
	image->filtered = NULL;
	image->filtered_size = 0;
}

static int png_color_channels(unsigned int color_type,
	unsigned int *channels)
{
	switch (color_type) {
	case PNG_COLOR_GRAYSCALE:
	case PNG_COLOR_INDEXED:
		*channels = 1;
		return TRUE;
	case PNG_COLOR_TRUECOLOR:
		*channels = 3;
		return TRUE;
	case PNG_COLOR_GRAYSCALE_ALPHA:
		*channels = 2;
		return TRUE;
	case PNG_COLOR_TRUECOLOR_ALPHA:
		*channels = 4;
		return TRUE;
	default:
		return FALSE;
	}
}

static int png_valid_bit_depth(unsigned int color_type,
	unsigned int bit_depth)
{
	switch (color_type) {
	case PNG_COLOR_GRAYSCALE:
		return (bit_depth == 1u) || (bit_depth == 2u) ||
		       (bit_depth == 4u) || (bit_depth == 8u) ||
		       (bit_depth == 16u);
	case PNG_COLOR_TRUECOLOR:
	case PNG_COLOR_GRAYSCALE_ALPHA:
	case PNG_COLOR_TRUECOLOR_ALPHA:
		return (bit_depth == 8u) || (bit_depth == 16u);
	case PNG_COLOR_INDEXED:
		return (bit_depth == 1u) || (bit_depth == 2u) ||
		       (bit_depth == 4u) || (bit_depth == 8u);
	default:
		return FALSE;
	}
}

static int png_pass_dimension(uint32_t full, unsigned int start,
	unsigned int step, size_t *dimension)
{
	size_t remaining;

	if (full <= start) {
		*dimension = 0;
		return TRUE;
	}
	remaining = (size_t)full - start;
	*dimension = 1u + ((remaining - 1u) / step);
	return TRUE;
}

static int png_row_size(const PNG_IMAGE *image, size_t width,
	size_t *row_size)
{
	size_t bits;

	if (!png_multiply_size(width, image->channels, &bits) ||
	    !png_multiply_size(bits, image->bit_depth, &bits))
		return FALSE;
	if (!png_add_size(bits, 7u, &bits))
		return FALSE;
	*row_size = bits / 8u;
	return TRUE;
}

static int png_calculate_filtered_size(const PNG_IMAGE *image,
	size_t *filtered_size)
{
	static const unsigned char start_x[7] = { 0, 4, 0, 2, 0, 1, 0 };
	static const unsigned char start_y[7] = { 0, 0, 4, 0, 2, 0, 1 };
	static const unsigned char step_x[7] = { 8, 8, 4, 4, 2, 2, 1 };
	static const unsigned char step_y[7] = { 8, 8, 8, 4, 4, 2, 2 };
	size_t total = 0;
	size_t pass_width;
	size_t pass_height;
	size_t row_size;
	size_t pass_size;
	unsigned int pass;
	unsigned int pass_count = image->interlace ? 7u : 1u;

	for (pass = 0; pass < pass_count; pass++) {
		unsigned int px = image->interlace ? start_x[pass] : 0u;
		unsigned int py = image->interlace ? start_y[pass] : 0u;
		unsigned int dx = image->interlace ? step_x[pass] : 1u;
		unsigned int dy = image->interlace ? step_y[pass] : 1u;

		png_pass_dimension(image->width, px, dx, &pass_width);
		png_pass_dimension(image->height, py, dy, &pass_height);
		if ((pass_width == 0) || (pass_height == 0))
			continue;
		if (!png_row_size(image, pass_width, &row_size) ||
		    !png_add_size(row_size, 1u, &row_size) ||
		    !png_multiply_size(row_size, pass_height, &pass_size) ||
		    !png_add_size(total, pass_size, &total))
			return FALSE;
	}
	*filtered_size = total;
	return TRUE;
}

static int png_read_chunk_header(FILE *file, uint32_t *length,
	unsigned char type[4], uint32_t *crc)
{
	unsigned char header[8];
	unsigned int i;

	if (!png_read_exact(file, header, sizeof(header)))
		return FALSE;
	*length = png_read_be32(header);
	memcpy(type, header + 4, 4);
	if (*length > 0x7FFFFFFFu)
		return FALSE;
	for (i = 0; i < 4u; i++) {
		if (!(((type[i] >= 'A') && (type[i] <= 'Z')) ||
		      ((type[i] >= 'a') && (type[i] <= 'z'))))
			return FALSE;
	}
	/* The PNG chunk-type reserved bit must remain zero. */
	if (type[2] & 0x20u)
		return FALSE;
	*crc = png_crc32_update(0xFFFFFFFFu, type, 4);
	return TRUE;
}

static int png_finish_chunk(FILE *file, uint32_t crc)
{
	unsigned char bytes[4];

	if (!png_read_exact(file, bytes, sizeof(bytes)))
		return FALSE;
	return png_read_be32(bytes) == (crc ^ 0xFFFFFFFFu);
}

static int png_parse_ihdr(PNG_IMAGE *image, const unsigned char *data,
	size_t size)
{
	if (size != 13u)
		return FALSE;
	image->width = png_read_be32(data);
	image->height = png_read_be32(data + 4);
	image->bit_depth = data[8];
	image->color_type = data[9];
	if ((image->width == 0) || (image->height == 0) ||
	    (image->width > INT_MAX) || (image->height > INT_MAX))
		return FALSE;
	if (!png_color_channels(image->color_type, &image->channels) ||
	    !png_valid_bit_depth(image->color_type, image->bit_depth))
		return FALSE;
	if ((data[10] != 0) || (data[11] != 0) || (data[12] > 1u))
		return FALSE;
	image->interlace = data[12];
	return png_calculate_filtered_size(image, &image->filtered_size);
}

static int png_parse_plte(PNG_IMAGE *image, const unsigned char *data,
	size_t size)
{
	size_t maximum_entries;

	if ((size == 0) || ((size % 3u) != 0) || (size > sizeof(image->palette)))
		return FALSE;
	if ((image->color_type == PNG_COLOR_GRAYSCALE) ||
	    (image->color_type == PNG_COLOR_GRAYSCALE_ALPHA))
		return FALSE;
	image->palette_entries = (unsigned int)(size / 3u);
	if (image->color_type == PNG_COLOR_INDEXED) {
		maximum_entries = (size_t)1u << image->bit_depth;
		if (image->palette_entries > maximum_entries)
			return FALSE;
	}
	memcpy(image->palette, data, size);
	return TRUE;
}

static int png_parse_trns(PNG_IMAGE *image, const unsigned char *data,
	size_t size)
{
	switch (image->color_type) {
	case PNG_COLOR_GRAYSCALE:
		if (size != 2u)
			return FALSE;
		image->transparent_gray =
			((unsigned int)data[0] << 8) | data[1];
		if ((image->bit_depth < 16u) &&
		    (image->transparent_gray >= (1u << image->bit_depth)))
			return FALSE;
		image->has_transparent_gray = TRUE;
		return TRUE;
	case PNG_COLOR_TRUECOLOR:
		if (size != 6u)
			return FALSE;
		image->transparent_red =
			((unsigned int)data[0] << 8) | data[1];
		image->transparent_green =
			((unsigned int)data[2] << 8) | data[3];
		image->transparent_blue =
			((unsigned int)data[4] << 8) | data[5];
		if ((image->bit_depth == 8u) &&
		    ((image->transparent_red > 255u) ||
		     (image->transparent_green > 255u) ||
		     (image->transparent_blue > 255u)))
			return FALSE;
		image->has_transparent_color = TRUE;
		return TRUE;
	case PNG_COLOR_INDEXED:
		if ((image->palette_entries == 0) ||
		    (size == 0) ||
		    (size > image->palette_entries))
			return FALSE;
		memcpy(image->palette_alpha, data, size);
		image->palette_alpha_entries = (unsigned int)size;
		return TRUE;
	default:
		return FALSE;
	}
}

static int png_load_chunks(FILE *file, PNG_IMAGE *image)
{
	unsigned char signature[8];
	unsigned char type[4];
	unsigned char data[768];
	uint32_t length;
	uint32_t crc;
	size_t data_size;
	int saw_ihdr = FALSE;
	int saw_idat = FALSE;
	int ended_idat = FALSE;
	int saw_iend = FALSE;
	int result = FB_GFX3_FAILED;

	memset(image, 0, sizeof(*image));
	memset(image->palette_alpha, 255, sizeof(image->palette_alpha));
	if (!png_read_exact(file, signature, sizeof(signature)) ||
	    memcmp(signature, png_signature, sizeof(signature)) != 0)
		return FB_GFX3_FAILED;

	while (!saw_iend) {
		if (!png_read_chunk_header(file, &length, type, &crc))
			goto exit;
		data_size = length;

		if (memcmp(type, "IHDR", 4) == 0) {
			if (saw_ihdr || saw_idat || (length != 13u))
				goto exit;
			if (!png_read_exact(file, data, data_size))
				goto exit;
			crc = png_crc32_update(crc, data, data_size);
			if (!png_finish_chunk(file, crc) ||
			    !png_parse_ihdr(image, data, data_size))
				goto exit;
			saw_ihdr = TRUE;
			continue;
		}
		if (!saw_ihdr)
			goto exit;

		if (memcmp(type, "PLTE", 4) == 0) {
			if (saw_idat || (image->palette_entries != 0) ||
			    (data_size > sizeof(data)))
				goto exit;
			if (!png_read_exact(file, data, data_size))
				goto exit;
			crc = png_crc32_update(crc, data, data_size);
			if (!png_finish_chunk(file, crc) ||
			    !png_parse_plte(image, data, data_size))
				goto exit;
			continue;
		}
		if (memcmp(type, "tRNS", 4) == 0) {
			if (saw_idat || image->has_transparent_gray ||
			    image->has_transparent_color ||
			    (image->palette_alpha_entries != 0) ||
			    (data_size > sizeof(data)))
				goto exit;
			if (!png_read_exact(file, data, data_size))
				goto exit;
			crc = png_crc32_update(crc, data, data_size);
			if (!png_finish_chunk(file, crc) ||
			    !png_parse_trns(image, data, data_size))
				goto exit;
			continue;
		}
		if (memcmp(type, "IDAT", 4) == 0) {
			if (ended_idat)
				goto exit;
			if ((image->color_type == PNG_COLOR_INDEXED) &&
			    (image->palette_entries == 0))
				goto exit;
			if (data_size != 0) {
				if (!png_byte_buffer_reserve(&image->compressed,
				    data_size))
					return FB_GFX3_OUT_OF_MEMORY;
				if (!png_read_exact(file,
				    image->compressed.data + image->compressed.size,
				    data_size))
					goto exit;
				crc = png_crc32_update(crc,
					image->compressed.data +
					image->compressed.size,
					data_size);
				image->compressed.size += data_size;
			}
			if (!png_finish_chunk(file, crc))
				goto exit;
			saw_idat = TRUE;
			continue;
		}
		if (saw_idat)
			ended_idat = TRUE;

		if (memcmp(type, "IEND", 4) == 0) {
			if (!saw_idat || (length != 0u) ||
			    !png_finish_chunk(file, crc))
				goto exit;
			saw_iend = TRUE;
			continue;
		}

		/*
			An uppercase first type byte marks a critical chunk.  Unknown
			critical chunks affect image interpretation and cannot be skipped.
		*/
		if ((type[0] & 0x20u) == 0)
			goto exit;
		if (!png_skip_and_crc(file, data_size, &crc) ||
		    !png_finish_chunk(file, crc))
			goto exit;
	}

	if (image->compressed.size == 0)
		goto exit;
	image->filtered = (unsigned char *)malloc(
		image->filtered_size ? image->filtered_size : 1u);
	if (image->filtered == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}
	if (!png_zlib_decompress(image->compressed.data,
	    image->compressed.size, image->filtered, image->filtered_size))
		goto exit;

	result = FB_GFX3_OK;

exit:
	return result;
}

/* ------------------------------------------------------------------------- */
/* Scanline reconstruction and sample conversion                             */
/* ------------------------------------------------------------------------- */

static unsigned int png_paeth_predictor(unsigned int left, unsigned int above,
	unsigned int upper_left)
{
	int estimate = (int)left + (int)above - (int)upper_left;
	int left_distance = abs(estimate - (int)left);
	int above_distance = abs(estimate - (int)above);
	int diagonal_distance = abs(estimate - (int)upper_left);

	if ((left_distance <= above_distance) &&
	    (left_distance <= diagonal_distance))
		return left;
	if (above_distance <= diagonal_distance)
		return above;
	return upper_left;
}

static int png_unfilter_row(unsigned char *row, const unsigned char *previous,
	size_t row_size, size_t filter_stride, unsigned int filter)
{
	size_t i;
	unsigned int left;
	unsigned int above;
	unsigned int upper_left;

	if (filter > 4u)
		return FALSE;
	for (i = 0; i < row_size; i++) {
		left = (i >= filter_stride) ? row[i - filter_stride] : 0u;
		above = (previous != NULL) ? previous[i] : 0u;
		upper_left = ((previous != NULL) && (i >= filter_stride)) ?
			previous[i - filter_stride] : 0u;

		switch (filter) {
		case 0:
			break;
		case 1:
			row[i] = (unsigned char)(row[i] + left);
			break;
		case 2:
			row[i] = (unsigned char)(row[i] + above);
			break;
		case 3:
			row[i] = (unsigned char)(row[i] +
				((left + above) >> 1));
			break;
		case 4:
			row[i] = (unsigned char)(row[i] +
				png_paeth_predictor(left, above, upper_left));
			break;
		}
	}
	return TRUE;
}

static unsigned int png_read_sample(const unsigned char *row,
	size_t sample_index, unsigned int bit_depth)
{
	size_t bit_position;
	unsigned int shift;
	unsigned int mask;

	if (bit_depth == 8u)
		return row[sample_index];
	if (bit_depth == 16u) {
		sample_index *= 2u;
		return ((unsigned int)row[sample_index] << 8) |
		       row[sample_index + 1u];
	}

	bit_position = sample_index * bit_depth;
	shift = 8u - bit_depth -
		(unsigned int)(bit_position & 7u);
	mask = (1u << bit_depth) - 1u;
	return (row[bit_position >> 3] >> shift) & mask;
}

static unsigned int png_scale_sample(unsigned int sample,
	unsigned int bit_depth)
{
	unsigned int maximum;

	if (bit_depth == 8u)
		return sample;
	if (bit_depth == 16u)
		return (sample * 255u + 32767u) / 65535u;
	maximum = (1u << bit_depth) - 1u;
	return (sample * 255u + (maximum / 2u)) / maximum;
}

static int png_get_rgba(const PNG_IMAGE *image, const unsigned char *row,
	size_t pixel, unsigned int *red, unsigned int *green,
	unsigned int *blue, unsigned int *alpha, unsigned int *index)
{
	size_t sample = pixel * image->channels;
	unsigned int gray;
	unsigned int raw_red;
	unsigned int raw_green;
	unsigned int raw_blue;

	*alpha = 255u;
	*index = 0;
	switch (image->color_type) {
	case PNG_COLOR_GRAYSCALE:
		gray = png_read_sample(row, sample, image->bit_depth);
		*red = *green = *blue =
			png_scale_sample(gray, image->bit_depth);
		if (image->has_transparent_gray &&
		    (gray == image->transparent_gray))
			*alpha = 0;
		return TRUE;

	case PNG_COLOR_TRUECOLOR:
		raw_red = png_read_sample(row, sample, image->bit_depth);
		raw_green = png_read_sample(row, sample + 1u, image->bit_depth);
		raw_blue = png_read_sample(row, sample + 2u, image->bit_depth);
		*red = png_scale_sample(raw_red, image->bit_depth);
		*green = png_scale_sample(raw_green, image->bit_depth);
		*blue = png_scale_sample(raw_blue, image->bit_depth);
		if (image->has_transparent_color &&
		    (raw_red == image->transparent_red) &&
		    (raw_green == image->transparent_green) &&
		    (raw_blue == image->transparent_blue))
			*alpha = 0;
		return TRUE;

	case PNG_COLOR_INDEXED:
		*index = png_read_sample(row, pixel, image->bit_depth);
		if (*index >= image->palette_entries)
			return FALSE;
		*red = image->palette[*index * 3u];
		*green = image->palette[*index * 3u + 1u];
		*blue = image->palette[*index * 3u + 2u];
		if (*index < image->palette_alpha_entries)
			*alpha = image->palette_alpha[*index];
		return TRUE;

	case PNG_COLOR_GRAYSCALE_ALPHA:
		gray = png_read_sample(row, sample, image->bit_depth);
		*red = *green = *blue =
			png_scale_sample(gray, image->bit_depth);
		*alpha = png_scale_sample(
			png_read_sample(row, sample + 1u, image->bit_depth),
			image->bit_depth);
		return TRUE;

	case PNG_COLOR_TRUECOLOR_ALPHA:
		*red = png_scale_sample(
			png_read_sample(row, sample, image->bit_depth),
			image->bit_depth);
		*green = png_scale_sample(
			png_read_sample(row, sample + 1u, image->bit_depth),
			image->bit_depth);
		*blue = png_scale_sample(
			png_read_sample(row, sample + 2u, image->bit_depth),
			image->bit_depth);
		*alpha = png_scale_sample(
			png_read_sample(row, sample + 3u, image->bit_depth),
			image->bit_depth);
		return TRUE;
	default:
		return FALSE;
	}
}

static void png_store_pixel(unsigned char *destination, int bpp,
	unsigned int red, unsigned int green, unsigned int blue,
	unsigned int alpha, unsigned int index)
{
	uint16_t color16;
	uint32_t color32;

	if (bpp == 1) {
		destination[0] = (unsigned char)index;
	} else if (bpp == 2) {
		color16 = (uint16_t)(
			((red >> 3) << 11) |
			((green >> 2) << 5) |
			(blue >> 3));
		memcpy(destination, &color16, sizeof(color16));
	} else {
		color32 =
			(alpha << 24) | (red << 16) | (green << 8) | blue;
		memcpy(destination, &color32, sizeof(color32));
	}
}

static int png_prepare_palette(const PNG_IMAGE *image, uint32_t bpp,
	void *pal, uint32_t palette[256], uint32_t *palette_entries)
{
	unsigned int i;
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int entries = 0;

	memset(palette, 0, 256u * sizeof(*palette));
	if (image->color_type == PNG_COLOR_INDEXED) {
		entries = image->palette_entries;
		for (i = 0; i < entries; i++) {
			red = image->palette[i * 3u];
			green = image->palette[i * 3u + 1u];
			blue = image->palette[i * 3u + 2u];
			palette[i] = red | (green << 8) | (blue << 16);
		}
	} else if (image->color_type == PNG_COLOR_GRAYSCALE) {
		entries = 256;
		for (i = 0; i < entries; i++)
			palette[i] = i | (i << 8) | (i << 16);
	}

	if ((bpp == 1) && (entries == 0))
		return FB_GFX3_INVALID;
	*palette_entries = entries;

	if ((pal != NULL) && (entries != 0)) {
		unsigned int *destination = (unsigned int *)pal;
		for (i = 0; i < entries; i++)
			destination[i] = (palette[i] >> 2) & 0x3F3F3Fu;
	}
	return FB_GFX3_OK;
}

static int png_prepare_destination(const PNG_IMAGE *image, void *dest,
	FB_GFX3_FILE_VIEW *view, uint32_t *width, uint32_t *height,
	uint32_t *bpp)
{
	int result;

	result = fb_gfx3_file_prepare_view_locked(dest, dest == NULL, view);
	if (result != FB_GFX3_OK)
		return result;
	*width = (view->width < image->width) ? view->width : image->width;
	*height = (view->height < image->height) ? view->height : image->height;
	*bpp = view->bytes_per_pixel;
	if ((*width == 0u) || (*height == 0u) ||
	    !((*bpp == 1u) || (*bpp == 2u) || (*bpp == 4u))) {
		fb_gfx3_file_release_view(view);
		return FB_GFX3_INVALID;
	}
	return FB_GFX3_OK;
}

static int png_convert_passes(FB_GFX3_FILE_VIEW *view, PNG_IMAGE *image,
	uint32_t output_width, uint32_t output_height, uint32_t output_bpp)
{
	static const unsigned char start_x[7] = { 0, 4, 0, 2, 0, 1, 0 };
	static const unsigned char start_y[7] = { 0, 0, 4, 0, 2, 0, 1 };
	static const unsigned char step_x[7] = { 8, 8, 4, 4, 2, 2, 1 };
	static const unsigned char step_y[7] = { 8, 8, 8, 4, 4, 2, 2 };
	unsigned char *stream = image->filtered;
	unsigned char *end = image->filtered + image->filtered_size;
	unsigned char *row;
	const unsigned char *previous;
	size_t pass_width;
	size_t pass_height;
	size_t row_size;
	size_t filter_stride;
	size_t pass_y;
	size_t pass_x;
	unsigned int pass;
	unsigned int pass_count = image->interlace ? 7u : 1u;
	unsigned int filter;
	unsigned int x;
	unsigned int y;
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int alpha;
	unsigned int index;

	filter_stride =
		((image->channels * image->bit_depth) + 7u) / 8u;
	if (filter_stride == 0)
		filter_stride = 1;

	for (pass = 0; pass < pass_count; pass++) {
		unsigned int px = image->interlace ? start_x[pass] : 0u;
		unsigned int py = image->interlace ? start_y[pass] : 0u;
		unsigned int dx = image->interlace ? step_x[pass] : 1u;
		unsigned int dy = image->interlace ? step_y[pass] : 1u;

		png_pass_dimension(image->width, px, dx, &pass_width);
		png_pass_dimension(image->height, py, dy, &pass_height);
		if ((pass_width == 0) || (pass_height == 0))
			continue;
		if (!png_row_size(image, pass_width, &row_size))
			return FALSE;
		previous = NULL;

		for (pass_y = 0; pass_y < pass_height; pass_y++) {
			if ((size_t)(end - stream) < (row_size + 1u))
				return FALSE;
			filter = *stream++;
			row = stream;
			if (!png_unfilter_row(row, previous, row_size,
			    filter_stride, filter))
				return FALSE;

			y = py + (unsigned int)pass_y * dy;
			if (y < output_height) {
				unsigned char *destination_row = view->pixels +
					((size_t)y * view->pitch);

				for (pass_x = 0; pass_x < pass_width; pass_x++) {
					x = px + (unsigned int)pass_x * dx;
					if (x >= output_width)
						continue;
					if (!png_get_rgba(image, row, pass_x,
					    &red, &green, &blue, &alpha, &index))
						return FALSE;
					if ((output_bpp == 1) &&
					    (image->color_type ==
					     PNG_COLOR_GRAYSCALE))
						index = red;
					png_store_pixel(destination_row +
						((size_t)x *
						 (unsigned int)output_bpp),
						output_bpp, red, green, blue,
						alpha, index);
				}
			}

			previous = row;
			stream += row_size;
		}
	}
	return stream == end;
}

int fb_gfx3_png_load_locked(FILE *file, void *destination, void *palette_arg)
{
	PNG_IMAGE image;
	FB_GFX3_FILE_VIEW view;
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_IMAGE_VIEW destination_view;
	uint32_t palette[256];
	uint32_t palette_entries;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t i;
	int result;

	memset(&image, 0, sizeof(image));
	memset(&view, 0, sizeof(view));
	if (file == NULL)
		return FB_GFX3_INVALID;
	result = png_load_chunks(file, &image);
	if (result != FB_GFX3_OK) {
		png_image_release(&image);
		return result;
	}

	result = png_prepare_destination(&image, destination, &view,
		&width, &height, &bpp);
	if (result != FB_GFX3_OK)
		goto exit;
	result = png_prepare_palette(&image, bpp, palette_arg, palette,
		&palette_entries);
	if (result != FB_GFX3_OK)
		goto exit;

	state = fb_gfx3_api_get_draw_state_locked();
	if ((palette_arg == NULL) && (palette_entries != 0u) &&
	    (state != NULL) && (state->mode != NULL)) {
		for (i = 0; i < palette_entries; ++i)
			state->mode->palette[i] = palette[i];
		result = fb_gfx3_context_set_palette(&state->mode->context,
			state->mode->palette);
	}
	if ((result == FB_GFX3_OK) &&
	    !png_convert_passes(&view, &image, width, height, bpp))
		result = FB_GFX3_FAILED;
	if (result == FB_GFX3_OK)
		result = fb_gfx3_file_commit_screen_locked(&view, width, height);
	if ((result == FB_GFX3_OK) && (destination != NULL) &&
	    (fb_gfx3_image_parse(destination, &destination_view) == FB_GFX3_OK))
		fb_gfx3_image_cache_metadata_touch(&destination_view);

exit:
	fb_gfx3_file_release_view(&view);
	png_image_release(&image);
	return result;
}

int fb_gfx3_png_load_pixels_locked(FILE *file, uint32_t depth,
	unsigned char **pixels, uint32_t *width, uint32_t *height,
	uint32_t *pitch)
{
	PNG_IMAGE image;
	FB_GFX3_FILE_VIEW view;
	size_t allocation_size;
	uint32_t bytes_per_pixel;
	int result;

	memset(&image, 0, sizeof(image));
	memset(&view, 0, sizeof(view));
	if (pixels != NULL)
		*pixels = NULL;
	if (width != NULL)
		*width = 0u;
	if (height != NULL)
		*height = 0u;
	if (pitch != NULL)
		*pitch = 0u;
	if ((file == NULL) || (pixels == NULL) || (width == NULL) ||
	    (height == NULL) || (pitch == NULL) ||
	    !((depth == 8u) || (depth == 16u) || (depth == 32u)))
		return FB_GFX3_INVALID;

	result = png_load_chunks(file, &image);
	if (result != FB_GFX3_OK)
		goto exit;
	bytes_per_pixel = (depth + 7u) / 8u;
	if ((bytes_per_pixel == 1u) &&
	    !((image.color_type == PNG_COLOR_INDEXED) ||
	      (image.color_type == PNG_COLOR_GRAYSCALE))) {
		result = FB_GFX3_INVALID;
		goto exit;
	}
	if ((image.width > (UINT32_MAX / bytes_per_pixel)) ||
	    (fb_gfx3_size_multiply(
	     (size_t)image.width * bytes_per_pixel, image.height,
	     &allocation_size) != FB_GFX3_OK)) {
		result = FB_GFX3_INVALID;
		goto exit;
	}
	view.width = image.width;
	view.height = image.height;
	view.bytes_per_pixel = bytes_per_pixel;
	view.pitch = image.width * bytes_per_pixel;
	view.allocation = (unsigned char *)malloc(allocation_size);
	if (view.allocation == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto exit;
	}
	view.pixels = view.allocation;
	if (!png_convert_passes(&view, &image, view.width, view.height,
	    view.bytes_per_pixel)) {
		result = FB_GFX3_FAILED;
		goto exit;
	}
	*pixels = view.allocation;
	*width = view.width;
	*height = view.height;
	*pitch = view.pitch;
	view.allocation = NULL;
	view.pixels = NULL;
	result = FB_GFX3_OK;

exit:
	fb_gfx3_file_release_view(&view);
	png_image_release(&image);
	return result;
}

/* ------------------------------------------------------------------------- */
/* PNG scanline encoder                                                      */
/* ------------------------------------------------------------------------- */

typedef struct PNG_SAVE_IMAGE {
	unsigned int width;
	unsigned int height;
	unsigned int input_bpp;
	unsigned int input_pitch;
	unsigned int color_type;
	unsigned int channels;
	unsigned char *pixels;
	const unsigned int *palette;
	unsigned int converted_palette[256];
} PNG_SAVE_IMAGE;

static int png_prepare_source(const FB_GFX3_FILE_VIEW *view, void *pal,
	int bitsperpixel, PNG_SAVE_IMAGE *image)
{
	FB_GFX3_DRAW_STATE *state;
	unsigned int i;
	const unsigned int *source_palette = (const unsigned int *)pal;

	memset(image, 0, sizeof(*image));
	if (view == NULL)
		return FB_GFX3_INVALID;
	image->width = view->width;
	image->height = view->height;
	image->input_bpp = view->bytes_per_pixel;
	image->input_pitch = view->pitch;
	image->pixels = view->pixels;

	if ((image->width == 0) || (image->height == 0) ||
	    (image->width > INT_MAX) || (image->height > INT_MAX) ||
	    (image->pixels == NULL) ||
	    !((image->input_bpp == 1u) || (image->input_bpp == 2u) ||
	      (image->input_bpp == 4u)) ||
	    (image->width > (UINT_MAX / image->input_bpp)) ||
	    (image->input_pitch < (image->width * image->input_bpp)))
		return FB_GFX3_INVALID;

	switch (image->input_bpp) {
	case 1:
		if (bitsperpixel > 8) {
			image->color_type = PNG_COLOR_TRUECOLOR;
			image->channels = 3;
		} else {
			image->color_type = PNG_COLOR_INDEXED;
			image->channels = 1;
		}
		if (source_palette == NULL) {
			state = fb_gfx3_api_get_draw_state_locked();
			if ((state != NULL) && (state->mode != NULL)) {
				image->palette = state->mode->palette;
			} else {
				for (i = 0; i < 256u; ++i) {
					image->converted_palette[i] =
						i | (i << 8) | (i << 16);
				}
				image->palette = image->converted_palette;
			}
		} else {
			for (i = 0; i < 256u; i++) {
				image->converted_palette[i] =
					((source_palette[i] & 0x00003Fu) << 2) |
					((source_palette[i] & 0x003F00u) << 2) |
					((source_palette[i] & 0x3F0000u) << 2);
			}
			image->palette = image->converted_palette;
		}
		break;
	case 2:
		image->color_type = PNG_COLOR_TRUECOLOR;
		image->channels = 3;
		break;
	case 4:
		if (bitsperpixel == 24) {
			image->color_type = PNG_COLOR_TRUECOLOR;
			image->channels = 3;
		} else {
			image->color_type = PNG_COLOR_TRUECOLOR_ALPHA;
			image->channels = 4;
		}
		break;
	default:
		return FB_GFX3_INVALID;
	}
	return FB_GFX3_OK;
}

static void png_encode_source_row(const PNG_SAVE_IMAGE *image,
	unsigned int y, unsigned char *row)
{
	const unsigned char *source =
		image->pixels + ((size_t)y * image->input_pitch);
	uint16_t color16;
	uint32_t color32;
	unsigned int x;
	unsigned int color;
	unsigned int red;
	unsigned int green;
	unsigned int blue;

	for (x = 0; x < image->width; x++) {
		if (image->input_bpp == 1u) {
			if (image->color_type == PNG_COLOR_INDEXED) {
				*row++ = source[x];
				continue;
			}
			color = image->palette[source[x]];
			red = color & 0xFFu;
			green = (color >> 8) & 0xFFu;
			blue = (color >> 16) & 0xFFu;
		} else if (image->input_bpp == 2u) {
			memcpy(&color16, source + ((size_t)x * 2u),
				sizeof(color16));
			color = color16;
			red = ((color >> 11) & 0x1Fu) * 255u / 31u;
			green = ((color >> 5) & 0x3Fu) * 255u / 63u;
			blue = (color & 0x1Fu) * 255u / 31u;
		} else {
			memcpy(&color32, source + ((size_t)x * 4u),
				sizeof(color32));
			color = color32;
			red = (color >> 16) & 0xFFu;
			green = (color >> 8) & 0xFFu;
			blue = color & 0xFFu;
		}

		*row++ = (unsigned char)red;
		*row++ = (unsigned char)green;
		*row++ = (unsigned char)blue;
		if (image->color_type == PNG_COLOR_TRUECOLOR_ALPHA)
			*row++ = (unsigned char)(color >> 24);
	}
}

static unsigned char png_filter_byte(unsigned int filter,
	unsigned char value, unsigned int left, unsigned int above,
	unsigned int upper_left)
{
	switch (filter) {
	case 0:
		return value;
	case 1:
		return (unsigned char)(value - left);
	case 2:
		return (unsigned char)(value - above);
	case 3:
		return (unsigned char)(value - ((left + above) >> 1));
	default:
		return (unsigned char)(value -
			png_paeth_predictor(left, above, upper_left));
	}
}

static unsigned int png_filter_score(const unsigned char *row,
	const unsigned char *previous, size_t row_size, size_t stride,
	unsigned int filter)
{
	size_t i;
	unsigned int left;
	unsigned int above;
	unsigned int upper_left;
	unsigned int filtered;
	unsigned int score = 0;

	for (i = 0; i < row_size; i++) {
		left = (i >= stride) ? row[i - stride] : 0u;
		above = (previous != NULL) ? previous[i] : 0u;
		upper_left = ((previous != NULL) && (i >= stride)) ?
			previous[i - stride] : 0u;
		filtered = png_filter_byte(filter, row[i], left, above,
			upper_left);
		/*
			PNG encoders conventionally score bytes as signed deltas.  The
			saturating add prevents an enormous row from wrapping the score.
		*/
		filtered = (filtered < 128u) ? filtered : (256u - filtered);
		if (score > (UINT_MAX - filtered))
			return UINT_MAX;
		score += filtered;
	}
	return score;
}

static void png_apply_filter(unsigned char *destination,
	const unsigned char *row, const unsigned char *previous,
	size_t row_size, size_t stride, unsigned int filter)
{
	size_t i;
	unsigned int left;
	unsigned int above;
	unsigned int upper_left;

	*destination++ = (unsigned char)filter;
	for (i = 0; i < row_size; i++) {
		left = (i >= stride) ? row[i - stride] : 0u;
		above = (previous != NULL) ? previous[i] : 0u;
		upper_left = ((previous != NULL) && (i >= stride)) ?
			previous[i - stride] : 0u;
		destination[i] = png_filter_byte(filter, row[i], left, above,
			upper_left);
	}
}

static int png_encode_scanlines(const PNG_SAVE_IMAGE *image,
	unsigned char **filtered, size_t *filtered_size)
{
	unsigned char *output = NULL;
	unsigned char *current = NULL;
	unsigned char *previous = NULL;
	unsigned char *swap;
	size_t row_size;
	size_t row_record_size;
	size_t total_size;
	unsigned int y;
	unsigned int filter;
	unsigned int best_filter;
	unsigned int score;
	unsigned int best_score;

	if ((image == NULL) || (filtered == NULL) || (filtered_size == NULL) ||
	    (image->pixels == NULL) || (image->width == 0u) ||
	    (image->height == 0u) ||
	    !(((image->color_type == PNG_COLOR_INDEXED) &&
	       (image->channels == 1u)) ||
	      ((image->color_type == PNG_COLOR_TRUECOLOR) &&
	       (image->channels == 3u)) ||
	      ((image->color_type == PNG_COLOR_TRUECOLOR_ALPHA) &&
	       (image->channels == 4u))) ||
	    !((image->input_bpp == 1u) || (image->input_bpp == 2u) ||
	      (image->input_bpp == 4u)) ||
	    ((image->input_bpp == 1u) && (image->palette == NULL)))
		return FB_GFX3_INVALID;
	if ((size_t)image->width > (PNG_SIZE_MAX / image->channels))
		return FB_GFX3_INVALID;
	row_size = (size_t)image->width * image->channels;
	if (row_size == PNG_SIZE_MAX)
		return FB_GFX3_INVALID;
	row_record_size = row_size + 1u;
	if ((size_t)image->height > (PNG_SIZE_MAX / row_record_size))
		return FB_GFX3_INVALID;
	total_size = row_record_size * image->height;

	output = (unsigned char *)malloc(total_size);
	current = (unsigned char *)malloc(row_size);
	previous = (unsigned char *)malloc(row_size);
	if ((output == NULL) || (current == NULL) || (previous == NULL)) {
		free(previous);
		free(current);
		free(output);
		return FB_GFX3_OUT_OF_MEMORY;
	}

	for (y = 0; y < image->height; y++) {
		png_encode_source_row(image, y, current);
		best_filter = 0;
		best_score = UINT_MAX;
		for (filter = 0; filter <= 4u; filter++) {
			score = png_filter_score(current, (y == 0) ? NULL : previous,
				row_size, image->channels, filter);
			if (score < best_score) {
				best_score = score;
				best_filter = filter;
			}
		}
		png_apply_filter(output + ((size_t)y * row_record_size),
			current, (y == 0) ? NULL : previous, row_size,
			image->channels, best_filter);
		swap = previous;
		previous = current;
		current = swap;
	}

	free(previous);
	free(current);
	*filtered = output;
	*filtered_size = total_size;
	return FB_GFX3_OK;
}

static int png_write_file(FILE *file, const PNG_SAVE_IMAGE *image,
	const unsigned char *compressed, size_t compressed_size)
{
	unsigned char ihdr[13];
	unsigned char palette[256 * 3];
	unsigned int i;
	size_t offset;
	size_t block;

	png_write_be32(ihdr, image->width);
	png_write_be32(ihdr + 4, image->height);
	ihdr[8] = 8;
	ihdr[9] = (unsigned char)image->color_type;
	ihdr[10] = 0;
	ihdr[11] = 0;
	ihdr[12] = 0;

	if (fwrite(png_signature, sizeof(png_signature), 1, file) != 1 ||
	    !png_write_chunk(file, "IHDR", ihdr, sizeof(ihdr)))
		return FALSE;

	if (image->color_type == PNG_COLOR_INDEXED) {
		for (i = 0; i < 256u; i++) {
			palette[i * 3u] =
				(unsigned char)(image->palette[i] & 0xFFu);
			palette[i * 3u + 1u] =
				(unsigned char)((image->palette[i] >> 8) & 0xFFu);
			palette[i * 3u + 2u] =
				(unsigned char)((image->palette[i] >> 16) & 0xFFu);
		}
		if (!png_write_chunk(file, "PLTE", palette, sizeof(palette)))
			return FALSE;
	}

	offset = 0;
	while (offset < compressed_size) {
		block = compressed_size - offset;
		if (block > PNG_IDAT_CHUNK_SIZE)
			block = PNG_IDAT_CHUNK_SIZE;
		if (!png_write_chunk(file, "IDAT", compressed + offset, block))
			return FALSE;
		offset += block;
	}
	return png_write_chunk(file, "IEND", NULL, 0);
}

int fb_gfx3_png_save_locked(FILE *file, const FB_GFX3_FILE_VIEW *view,
	void *pal, int bitsperpixel)
{
	PNG_SAVE_IMAGE image;
	unsigned char *filtered = NULL;
	unsigned char *compressed = NULL;
	size_t filtered_size = 0;
	size_t compressed_size = 0;
	int result;

	if ((file == NULL) || (view == NULL))
		return FB_GFX3_INVALID;
	result = png_prepare_source(view, pal, bitsperpixel, &image);
	if (result != FB_GFX3_OK)
		goto exit;
	result = png_encode_scanlines(&image, &filtered, &filtered_size);
	if (result != FB_GFX3_OK)
		goto exit;
	result = png_zlib_compress(filtered, filtered_size, &compressed,
		&compressed_size);
	if (result != FB_GFX3_OK)
		goto exit;
	if (!png_write_file(file, &image, compressed, compressed_size))
		result = FB_GFX3_FAILED;

exit:
	free(compressed);
	free(filtered);
	return result;
}

/* end of gfx3_png.c */
