/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_file_api.c

    Purpose:

        Load and save graphics buffers without depending on gfxlib2's
        process-global framebuffer.

    Responsibilities:

        - implement FreeBASIC and QB-compatible raw BSAVE/BLOAD blocks
        - identify BMP and PNG files and route PNG work to its codec module
        - save 8, 16, and 32-bit images and screen pages as BMP files
        - decode bounded Windows RLE4/RLE8 palette streams before upload
        - load 1/4/8-bit indexed and direct-color Windows/OS2 BMP files
        - synchronize screen-page transfers through the gfxlib3 renderer

    This file intentionally does NOT contain:

        - PNG codec internals
        - JPEG and non-RLE BMP codecs
        - drawing or presentation logic
*/

#include "gfx3_api_internal.h"
#include "gfx3_file_api.h"
#include "gfx3_image.h"
#include "gfx3_png.h"

enum {
	FB_GFX3_FILE_PATH_MAX = 1024,
	FB_GFX3_BMP_COMPRESSION_RGB = 0,
	FB_GFX3_BMP_COMPRESSION_RLE8 = 1,
	FB_GFX3_BMP_COMPRESSION_RLE4 = 2,
	FB_GFX3_BMP_COMPRESSION_BITFIELDS = 3
};

/* ------------------------------------------------------------------------- */
/* FreeBASIC string paths                                                    */
/* ------------------------------------------------------------------------- */

static int file_api_copy_path(char *path, size_t capacity,
	FBSTRING *filename)
{
	ssize_t length;
	size_t copy_length;

	if ((path == NULL) || (capacity == 0u) || (filename == NULL) ||
	    (filename->data == NULL))
		return FALSE;
	length = FB_STRSIZE(filename);
	if (length < 0)
		return FALSE;
	copy_length = (size_t)length;
	if (copy_length >= capacity)
		copy_length = capacity - 1u;
	if (copy_length != 0u)
		memcpy(path, filename->data, copy_length);
	path[copy_length] = '\0';
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Little-endian file helpers                                                */
/* ------------------------------------------------------------------------- */

static int file_api_read_u8(FILE *file, uint8_t *value)
{
	int byte;

	if ((file == NULL) || (value == NULL))
		return FALSE;
	byte = fgetc(file);
	if (byte == EOF)
		return FALSE;
	*value = (uint8_t)byte;
	return TRUE;
}

static int file_api_read_u16(FILE *file, uint16_t *value)
{
	uint8_t bytes[2];

	if ((value == NULL) ||
	    !file_api_read_u8(file, &bytes[0]) ||
	    !file_api_read_u8(file, &bytes[1]))
		return FALSE;
	*value = (uint16_t)(((uint16_t)bytes[0]) |
		((uint16_t)bytes[1] << 8));
	return TRUE;
}

static int file_api_read_u32(FILE *file, uint32_t *value)
{
	uint8_t bytes[4];

	if ((value == NULL) ||
	    !file_api_read_u8(file, &bytes[0]) ||
	    !file_api_read_u8(file, &bytes[1]) ||
	    !file_api_read_u8(file, &bytes[2]) ||
	    !file_api_read_u8(file, &bytes[3]))
		return FALSE;
	*value = (uint32_t)bytes[0] |
		((uint32_t)bytes[1] << 8) |
		((uint32_t)bytes[2] << 16) |
		((uint32_t)bytes[3] << 24);
	return TRUE;
}

static int file_api_write_u8(FILE *file, uint8_t value)
{
	return (fputc((int)value, file) != EOF);
}

static int file_api_write_u16(FILE *file, uint16_t value)
{
	return file_api_write_u8(file, (uint8_t)value) &&
		file_api_write_u8(file, (uint8_t)(value >> 8));
}

static int file_api_write_u32(FILE *file, uint32_t value)
{
	return file_api_write_u8(file, (uint8_t)value) &&
		file_api_write_u8(file, (uint8_t)(value >> 8)) &&
		file_api_write_u8(file, (uint8_t)(value >> 16)) &&
		file_api_write_u8(file, (uint8_t)(value >> 24));
}

static int file_api_skip(FILE *file, uint32_t byte_count)
{
	if (byte_count > (uint32_t)LONG_MAX)
		return FALSE;
	return (fseek(file, (long)byte_count, SEEK_CUR) == 0);
}

/* ------------------------------------------------------------------------- */
/* RLE indexed BMP expansion                                                 */
/* ------------------------------------------------------------------------- */

static int file_api_decode_rle(FILE *file, uint32_t width, uint32_t height,
	uint16_t bits_per_pixel, uint8_t **destination)
{
	uint8_t *indices;
	size_t allocation_size;
	uint32_t x = 0u;
	uint32_t y = 0u;
	int complete = FALSE;

	if ((file == NULL) || (destination == NULL) || (width == 0u) ||
	    (height == 0u) || !((bits_per_pixel == 4u) ||
	    (bits_per_pixel == 8u)) ||
	    (fb_gfx3_size_multiply((size_t)width, height,
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	*destination = NULL;
	indices = (uint8_t *)calloc(1u, allocation_size);
	if (indices == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	while (!complete) {
		uint8_t count;
		uint8_t value;
		uint32_t i;

		if (!file_api_read_u8(file, &count) || !file_api_read_u8(file, &value))
			goto failed;
		if (count != 0u) {
			if ((y >= height) || (count > (width - x)))
				goto failed;
			for (i = 0u; i < count; ++i) {
				uint8_t index = (bits_per_pixel == 8u) ? value :
					(uint8_t)((value >> ((i & 1u) ? 0u : 4u)) & 0x0Fu);

				indices[((size_t)y * width) + x++] = index;
			}
			continue;
		}
		switch (value) {
		case 0u: /* End of scanline */
			x = 0u;
			if (++y > height)
				goto failed;
			break;
		case 1u: /* End of bitmap */
			complete = TRUE;
			break;
		case 2u: { /* Relative move */
			uint8_t delta_x;
			uint8_t delta_y;

			if (!file_api_read_u8(file, &delta_x) ||
			    !file_api_read_u8(file, &delta_y) || (y >= height) ||
			    (delta_y > ((height - 1u) - y) ||
			    (delta_x > (width - x))))
				goto failed;
			x += delta_x;
			y += delta_y;
			break;
		}
		default: { /* Absolute palette indexes */
			uint32_t byte_count = (bits_per_pixel == 8u) ? value :
				((uint32_t)value + 1u) / 2u;
			uint8_t packed = 0u;

			if ((y >= height) || (value > (width - x)))
				goto failed;
			for (i = 0u; i < value; ++i) {
				if ((i & ((bits_per_pixel == 8u) ? 0u : 1u)) == 0u &&
				    !file_api_read_u8(file, &packed))
					goto failed;
				indices[((size_t)y * width) + x++] =
					(bits_per_pixel == 8u) ? packed :
					(uint8_t)((packed >> ((i & 1u) ? 0u : 4u)) & 0x0Fu);
			}
			if ((byte_count & 1u) != 0u && !file_api_skip(file, 1u))
				goto failed;
			break;
		}
		}
	}
	*destination = indices;
	return FB_GFX3_OK;

failed:
	free(indices);
	return FB_GFX3_FAILED;
}

/* ------------------------------------------------------------------------- */
/* Shared image and screen views                                             */
/* ------------------------------------------------------------------------- */

static uint32_t file_api_bytes_per_pixel(uint32_t depth)
{
	return (depth + 7u) / 8u;
}

int fb_gfx3_file_prepare_view_locked(void *image, int read_screen,
	FB_GFX3_FILE_VIEW *view)
{
	FB_GFX3_IMAGE_VIEW image_view;
	FB_GFX3_DRAW_STATE *state;
	size_t allocation_size;
	int result;

	if (view == NULL)
		return FB_GFX3_INVALID;
	memset(view, 0, sizeof(*view));
	if (image != NULL) {
		result = fb_gfx3_image_parse(image, &image_view);
		if (result != FB_GFX3_OK)
			return result;
		view->pixels = image_view.pixels;
		view->width = image_view.width;
		view->height = image_view.height;
		view->pitch = image_view.pitch;
		view->bytes_per_pixel = image_view.bytes_per_pixel;
		return FB_GFX3_OK;
	}

	state = fb_gfx3_api_get_draw_state_locked();
	if ((state == NULL) || (state->mode == NULL) ||
	    (state->work_page >= state->mode->page_count))
		return FB_GFX3_INVALID;
	view->state = state;
	view->width = state->mode->width;
	view->height = state->mode->height;
	view->bytes_per_pixel = file_api_bytes_per_pixel(state->mode->depth);
	if ((view->width == 0u) || (view->height == 0u) ||
	    (view->bytes_per_pixel == 0) ||
	    (view->width > UINT32_MAX / view->bytes_per_pixel))
		return FB_GFX3_INVALID;
	view->pitch = view->width * view->bytes_per_pixel;
	if ((fb_gfx3_size_multiply(view->pitch, view->height,
	     &allocation_size) != FB_GFX3_OK) ||
	    (allocation_size == 0u) || (allocation_size > UINT32_MAX))
		return FB_GFX3_INVALID;
	view->allocation = (unsigned char *)malloc(allocation_size);
	if (view->allocation == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	view->pixels = view->allocation;
	if (read_screen) {
		/*
			PSET operations are deliberately batched until an ordering boundary.
			A file save is such a boundary: the downloaded page must include every
			graphics command that precedes BSAVE in the BASIC program.
		*/
		result = fb_gfx3_compat_flush_points_graphics_locked(state);
		if (result != FB_GFX3_OK) {
			free(view->allocation);
			memset(view, 0, sizeof(*view));
			return result;
		}
		result = fb_gfx3_surface_download(
			&state->mode->pages[state->work_page], 0, 0,
			view->width, view->height, view->pitch, view->pixels);
		if (result != FB_GFX3_OK) {
			free(view->allocation);
			memset(view, 0, sizeof(*view));
			return result;
		}
	} else {
		memset(view->pixels, 0, allocation_size);
	}
	return FB_GFX3_OK;
}

void fb_gfx3_file_release_view(FB_GFX3_FILE_VIEW *view)
{
	if (view == NULL)
		return;
	free(view->allocation);
	memset(view, 0, sizeof(*view));
}

int fb_gfx3_file_commit_screen_locked(FB_GFX3_FILE_VIEW *view,
	uint32_t width, uint32_t height)
{
	FB_GFX3_MODE *mode;
	int result;

	if ((view == NULL) || (view->state == NULL) ||
	    (view->state->mode == NULL))
		return FB_GFX3_OK;
	mode = view->state->mode;
	result = fb_gfx3_surface_upload(
		&mode->pages[view->state->work_page], 0, 0,
		width, height, view->pitch, view->pixels);
	if ((result == FB_GFX3_OK) && (mode->shadow_valid != NULL))
		mode->shadow_valid[view->state->work_page] = FALSE;
	if (result == FB_GFX3_OK)
		fb_gfx3_compat_invalidate_point_cache_graphics_locked(view->state);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Color conversion                                                          */
/* ------------------------------------------------------------------------- */

static uint32_t file_api_load_color(const unsigned char *pixel,
	uint32_t bytes_per_pixel, const uint32_t *palette)
{
	uint16_t value16;
	uint32_t value32;
	uint32_t red;
	uint32_t green;
	uint32_t blue;

	if (bytes_per_pixel == 1)
		return (palette != NULL) ? palette[pixel[0]] : pixel[0];
	if (bytes_per_pixel == 2) {
		memcpy(&value16, pixel, sizeof(value16));
		red = ((value16 >> 11) & 31u) * 255u / 31u;
		green = ((value16 >> 5) & 63u) * 255u / 63u;
		blue = (value16 & 31u) * 255u / 31u;
		return 0xFF000000u | (red << 16) | (green << 8) | blue;
	}
	memcpy(&value32, pixel, sizeof(value32));
	return value32;
}

static void file_api_store_color(unsigned char *pixel,
	uint32_t bytes_per_pixel, uint32_t color, uint8_t palette_index)
{
	uint16_t value16;

	if (bytes_per_pixel == 1) {
		pixel[0] = palette_index;
	} else if (bytes_per_pixel == 2) {
		value16 = (uint16_t)((((color >> 16) & 0xFFu) >> 3) << 11);
		value16 |= (uint16_t)((((color >> 8) & 0xFFu) >> 2) << 5);
		value16 |= (uint16_t)((color & 0xFFu) >> 3);
		memcpy(pixel, &value16, sizeof(value16));
	} else {
		memcpy(pixel, &color, sizeof(color));
	}
}

static int file_api_mask_info(uint32_t mask, uint32_t *shift,
	uint32_t *maximum)
{
	uint32_t local_shift = 0;
	uint32_t normalized;

	if ((mask == 0u) || (shift == NULL) || (maximum == NULL))
		return FALSE;
	while ((mask & 1u) == 0u) {
		mask >>= 1;
		local_shift++;
	}
	normalized = mask;
	if ((normalized & (normalized + 1u)) != 0u)
		return FALSE;
	*shift = local_shift;
	*maximum = normalized;
	return TRUE;
}

static uint32_t file_api_expand_masked(uint32_t value, uint32_t mask,
	uint32_t shift, uint32_t maximum)
{
	uint32_t component;
	uint64_t scaled;

	if ((mask == 0u) || (maximum == 0u))
		return 255u;
	component = (value & mask) >> shift;

	/*
		A legal 32-bit mask may use every bit of the source pixel.  Keep
		the normalized conversion in 64 bits so a full-width component does
		not wrap before it is reduced to the 0 through 255 API colour range.
	*/
	scaled = ((uint64_t)component * 255u) + ((uint64_t)maximum / 2u);
	return (uint32_t)(scaled / maximum);
}

/* ------------------------------------------------------------------------- */
/* BMP save                                                                  */
/* ------------------------------------------------------------------------- */

static int file_api_write_bmp_header(FILE *file, uint32_t width,
	uint32_t height, uint16_t bits_per_pixel, uint32_t row_pitch,
	uint32_t palette_entries)
{
	uint64_t image_size;
	uint64_t pixel_offset;
	uint64_t file_size;

	image_size = (uint64_t)row_pitch * height;
	pixel_offset = 14u + 40u + ((uint64_t)palette_entries * 4u);
	file_size = pixel_offset + image_size;
	if ((image_size > UINT32_MAX) || (pixel_offset > UINT32_MAX) ||
	    (file_size > UINT32_MAX))
		return FB_GFX3_INVALID;

	if (!file_api_write_u16(file, 0x4D42u) ||
	    !file_api_write_u32(file, (uint32_t)file_size) ||
	    !file_api_write_u16(file, 0) ||
	    !file_api_write_u16(file, 0) ||
	    !file_api_write_u32(file, (uint32_t)pixel_offset) ||
	    !file_api_write_u32(file, 40) ||
	    !file_api_write_u32(file, width) ||
	    !file_api_write_u32(file, height) ||
	    !file_api_write_u16(file, 1) ||
	    !file_api_write_u16(file, bits_per_pixel) ||
	    !file_api_write_u32(file, 0) ||
	    !file_api_write_u32(file, (uint32_t)image_size) ||
	    !file_api_write_u32(file, 0x0B12u) ||
	    !file_api_write_u32(file, 0x0B12u) ||
	    !file_api_write_u32(file, palette_entries) ||
	    !file_api_write_u32(file, palette_entries))
		return FB_GFX3_FAILED;
	return FB_GFX3_OK;
}

static int file_api_save_bmp(FILE *file, const FB_GFX3_FILE_VIEW *view,
	void *palette_argument, int requested_bits)
{
	uint32_t output_bytes;
	uint32_t row_bytes;
	uint32_t row_pitch;
	uint32_t palette[256];
	uint32_t palette_entries;
	unsigned char *row;
	const unsigned char *source;
	uint32_t x;
	uint32_t y;
	uint32_t color;
	int result;

	if ((file == NULL) || (view == NULL) || (view->pixels == NULL) ||
	    (view->width == 0) || (view->height == 0))
		return FB_GFX3_INVALID;
	if (view->bytes_per_pixel == 1)
		output_bytes = (requested_bits > 8) ? 3u : 1u;
	else if (view->bytes_per_pixel == 2)
		output_bytes = 3u;
	else if (view->bytes_per_pixel == 4)
		output_bytes = (requested_bits == 24) ? 3u : 4u;
	else
		return FB_GFX3_INVALID;
	if ((view->width > UINT32_MAX / output_bytes) ||
	    (view->width * output_bytes > UINT32_MAX - 3u))
		return FB_GFX3_INVALID;
	row_bytes = view->width * output_bytes;
	row_pitch = (row_bytes + 3u) & ~3u;
	palette_entries = (output_bytes == 1) ? 256u : 0u;

	for (x = 0; x < 256u; ++x) {
		if (palette_argument != NULL) {
			const uint32_t *source_palette =
				(const uint32_t *)palette_argument;
			uint32_t value = source_palette[x];

			palette[x] = ((value & 0x3F0000u) << 2) |
				((value & 0x003F00u) << 2) |
				((value & 0x00003Fu) << 2);
		} else if ((view->state != NULL) &&
		           (view->state->mode != NULL)) {
			palette[x] = view->state->mode->palette[x];
		} else {
			palette[x] = 0xFF000000u | (x << 16) | (x << 8) | x;
		}
	}
	result = file_api_write_bmp_header(file, view->width, view->height,
		(uint16_t)(output_bytes * 8u), row_pitch, palette_entries);
	if (result != FB_GFX3_OK)
		return result;
	if (palette_entries != 0) {
		for (x = 0; x < palette_entries; ++x) {
			color = palette[x];
			/* BMP palette records are B, G, R, reserved; mode->palette is RGB. */
			if (!file_api_write_u8(file, (uint8_t)(color >> 16)) ||
			    !file_api_write_u8(file, (uint8_t)(color >> 8)) ||
			    !file_api_write_u8(file, (uint8_t)color) ||
			    !file_api_write_u8(file, 0))
				return FB_GFX3_FAILED;
		}
	}

	row = (unsigned char *)calloc(1, row_pitch);
	if (row == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (y = view->height; y > 0; --y) {
		memset(row, 0, row_pitch);
		source = view->pixels + ((size_t)(y - 1u) * view->pitch);
		if (output_bytes == 1) {
			memcpy(row, source, view->width);
		} else {
			for (x = 0; x < view->width; ++x) {
				color = file_api_load_color(source +
					((size_t)x * view->bytes_per_pixel),
					view->bytes_per_pixel, palette);
				row[(size_t)x * output_bytes] = (uint8_t)color;
				row[(size_t)x * output_bytes + 1u] =
					(uint8_t)(color >> 8);
				row[(size_t)x * output_bytes + 2u] =
					(uint8_t)(color >> 16);
				if (output_bytes == 4)
					row[(size_t)x * output_bytes + 3u] =
						(uint8_t)(color >> 24);
			}
		}
		if (fwrite(row, 1, row_pitch, file) != row_pitch) {
			free(row);
			return FB_GFX3_FAILED;
		}
	}
	free(row);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* BMP load                                                                  */
/* ------------------------------------------------------------------------- */

static int file_api_load_bmp(FILE *file, void *destination,
	void *palette_argument, uint32_t output_depth,
	FB_GFX3_FILE_VIEW *output_view)
{
	FB_GFX3_FILE_VIEW view;
	uint16_t signature;
	uint16_t reserved;
	uint16_t planes;
	uint16_t bits_per_pixel;
	uint32_t ignored;
	uint32_t pixel_offset;
	uint32_t header_size;
	uint32_t width_value;
	uint32_t height_value;
	uint32_t compression;
	uint32_t colors_used;
	uint16_t core_width;
	uint16_t core_height;
	uint32_t source_palette[256];
	uint32_t masks[4] = { 0u, 0u, 0u, 0u };
	uint32_t shifts[4] = { 0u, 0u, 0u, 0u };
	uint32_t maximums[4] = { 0u, 0u, 0u, 0u };
	uint32_t palette_entries;
	uint32_t source_row_pitch;
	uint32_t copy_width;
	uint32_t copy_height;
	unsigned char *source_row;
	unsigned char *destination_row;
	uint8_t *rle_indices;
	int32_t signed_width;
	int32_t signed_height;
	uint32_t x;
	uint32_t y;
	uint32_t color;
	uint8_t index;
	int top_down;
	int os2_core;
	int rle_compressed;
	int result;

	memset(&view, 0, sizeof(view));
	rle_indices = NULL;
	if (!file_api_read_u16(file, &signature) ||
	    !file_api_read_u32(file, &ignored) ||
	    !file_api_read_u16(file, &reserved) ||
	    !file_api_read_u16(file, &reserved) ||
	    !file_api_read_u32(file, &pixel_offset) ||
	    !file_api_read_u32(file, &header_size) ||
	    (signature != 0x4D42u))
		return FB_GFX3_FAILED;
	os2_core = (header_size == 12u);
	if (os2_core) {
		if (!file_api_read_u16(file, &core_width) ||
		    !file_api_read_u16(file, &core_height) ||
		    !file_api_read_u16(file, &planes) ||
		    !file_api_read_u16(file, &bits_per_pixel))
			return FB_GFX3_FAILED;
		width_value = core_width;
		height_value = core_height;
		compression = FB_GFX3_BMP_COMPRESSION_RGB;
		colors_used = 0u;
	} else if (header_size < 40u) {
		return FB_GFX3_UNSUPPORTED;
	} else if (!file_api_read_u32(file, &width_value) ||
		!file_api_read_u32(file, &height_value) ||
		!file_api_read_u16(file, &planes) ||
		!file_api_read_u16(file, &bits_per_pixel) ||
		!file_api_read_u32(file, &compression) ||
		!file_api_read_u32(file, &ignored) ||
		!file_api_read_u32(file, &ignored) ||
		!file_api_read_u32(file, &ignored) ||
		!file_api_read_u32(file, &colors_used) ||
		!file_api_read_u32(file, &ignored)) {
		return FB_GFX3_FAILED;
	}
	memcpy(&signed_width, &width_value, sizeof(signed_width));
	memcpy(&signed_height, &height_value, sizeof(signed_height));
	if ((planes != 1) || (signed_width <= 0) || (signed_height == 0) ||
	    (signed_height == INT32_MIN) ||
	    !((compression == FB_GFX3_BMP_COMPRESSION_RGB) ||
	      (compression == FB_GFX3_BMP_COMPRESSION_RLE8) ||
	      (compression == FB_GFX3_BMP_COMPRESSION_RLE4) ||
	      (compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS)) ||
	    !(os2_core ? ((bits_per_pixel == 1) || (bits_per_pixel == 4) ||
		(bits_per_pixel == 8) || (bits_per_pixel == 24)) :
		((bits_per_pixel == 1) || (bits_per_pixel == 4) ||
		 (bits_per_pixel == 8) || (bits_per_pixel == 16) ||
		 (bits_per_pixel == 24) || (bits_per_pixel == 32))))
		return FB_GFX3_UNSUPPORTED;
	if ((compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS) &&
	    !((bits_per_pixel == 16) || (bits_per_pixel == 32)))
		return FB_GFX3_UNSUPPORTED;
	if ((uint64_t)pixel_offset < (14u + (uint64_t)header_size))
		return FB_GFX3_FAILED;
	top_down = !os2_core && (signed_height < 0);
	if (top_down)
		signed_height = -signed_height;
	rle_compressed = (compression == FB_GFX3_BMP_COMPRESSION_RLE8) ||
		(compression == FB_GFX3_BMP_COMPRESSION_RLE4);
	if (rle_compressed && (os2_core || top_down ||
	    ((compression == FB_GFX3_BMP_COMPRESSION_RLE8) &&
	     (bits_per_pixel != 8u)) ||
	    ((compression == FB_GFX3_BMP_COMPRESSION_RLE4) &&
	     (bits_per_pixel != 4u))))
		return FB_GFX3_UNSUPPORTED;
	if (header_size > 40u) {
		uint32_t extension_size = header_size - 40u;

		if (compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS) {
			if ((extension_size < 12u) ||
			    !file_api_read_u32(file, &masks[0]) ||
			    !file_api_read_u32(file, &masks[1]) ||
			    !file_api_read_u32(file, &masks[2]))
				return FB_GFX3_FAILED;
			extension_size -= 12u;
			if ((extension_size >= 4u) &&
			    !file_api_read_u32(file, &masks[3]))
				return FB_GFX3_FAILED;
			if (extension_size >= 4u)
				extension_size -= 4u;
		}
		if (!file_api_skip(file, extension_size))
			return FB_GFX3_FAILED;
	} else if (compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS) {
		if ((pixel_offset < (14u + 40u + 12u)) ||
		    !file_api_read_u32(file, &masks[0]) ||
		    !file_api_read_u32(file, &masks[1]) ||
		    !file_api_read_u32(file, &masks[2]))
			return FB_GFX3_FAILED;

		/*
			The original 40-byte Windows information header carries RGB
			masks immediately after the header.  Some 32-bit writers append
			one more DWORD for alpha before the pixel array.  The offset is
			the only reliable boundary in this header form, so consume that
			fourth field only when there is room for it.
		*/
		if ((bits_per_pixel == 32u) &&
		    (pixel_offset >= (14u + 40u + 16u)) &&
		    !file_api_read_u32(file, &masks[3]))
			return FB_GFX3_FAILED;
	}
	if (compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS) {
		if (!file_api_mask_info(masks[0], &shifts[0], &maximums[0]) ||
		    !file_api_mask_info(masks[1], &shifts[1], &maximums[1]) ||
		    !file_api_mask_info(masks[2], &shifts[2], &maximums[2]) ||
		    ((masks[0] & masks[1]) != 0u) ||
		    ((masks[0] & masks[2]) != 0u) ||
		    ((masks[1] & masks[2]) != 0u))
			return FB_GFX3_FAILED;
		if ((masks[3] != 0u) &&
		    (!file_api_mask_info(masks[3], &shifts[3], &maximums[3]) ||
		     ((masks[3] & (masks[0] | masks[1] | masks[2])) != 0u)))
			return FB_GFX3_FAILED;
	}

	for (x = 0; x < 256u; ++x)
		source_palette[x] = 0xFF000000u | (x << 16) | (x << 8) | x;
	if (bits_per_pixel <= 8) {
		uint64_t palette_end;
		uint32_t palette_entry_size = os2_core ? 3u : 4u;

		palette_entries = (colors_used == 0) ?
			(1u << bits_per_pixel) : colors_used;
		if (palette_entries > 256u)
			return FB_GFX3_FAILED;
		palette_end = 14u + (uint64_t)header_size +
			((uint64_t)palette_entries * palette_entry_size);
		if (palette_end > pixel_offset)
			return FB_GFX3_FAILED;
		for (x = 0; x < palette_entries; ++x) {
			uint8_t blue;
			uint8_t green;
			uint8_t red;
			uint8_t alpha_ignored;

			if (!file_api_read_u8(file, &blue) ||
			    !file_api_read_u8(file, &green) ||
			    !file_api_read_u8(file, &red) ||
			    (!os2_core && !file_api_read_u8(file, &alpha_ignored)))
				return FB_GFX3_FAILED;
			source_palette[x] = 0xFF000000u |
				((uint32_t)red << 16) |
				((uint32_t)green << 8) | blue;
			if (palette_argument != NULL) {
				uint32_t *output_palette =
					(uint32_t *)palette_argument;

				output_palette[x] = ((uint32_t)(red >> 2) << 16) |
					((uint32_t)(green >> 2) << 8) |
					(uint32_t)(blue >> 2);
			}
		}
	}
	if ((uint64_t)(uint32_t)signed_width * bits_per_pixel >
	    ((uint64_t)UINT32_MAX * 8u) - 31u)
		return FB_GFX3_INVALID;
	source_row_pitch = (uint32_t)((((uint64_t)(uint32_t)signed_width *
		bits_per_pixel) + 31u) / 32u * 4u);
	if (output_view != NULL) {
		size_t allocation_size;

		view.width = (uint32_t)signed_width;
		view.height = (uint32_t)signed_height;
		view.bytes_per_pixel = file_api_bytes_per_pixel(output_depth);
		if (!((output_depth == 8u) || (output_depth == 16u) ||
		      (output_depth == 32u)) || (view.bytes_per_pixel == 0u) ||
		    (view.width > UINT32_MAX / view.bytes_per_pixel) ||
		    (fb_gfx3_size_multiply(view.width * view.bytes_per_pixel,
		     view.height, &allocation_size) != FB_GFX3_OK))
			return FB_GFX3_INVALID;
		view.pitch = view.width * view.bytes_per_pixel;
		view.allocation = (unsigned char *)calloc(1u, allocation_size);
		if (view.allocation == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		view.pixels = view.allocation;
		result = FB_GFX3_OK;
	} else {
		result = fb_gfx3_file_prepare_view_locked(destination,
			destination == NULL, &view);
	}
	if (result != FB_GFX3_OK)
		return result;
	copy_width = ((uint32_t)signed_width < view.width) ?
		(uint32_t)signed_width : view.width;
	copy_height = ((uint32_t)signed_height < view.height) ?
		(uint32_t)signed_height : view.height;
	if ((view.bytes_per_pixel == 1) && (bits_per_pixel > 8)) {
		fb_gfx3_file_release_view(&view);
		return FB_GFX3_UNSUPPORTED;
	}
	source_row = NULL;
	if ((pixel_offset > (uint32_t)LONG_MAX) ||
	    (fseek(file, (long)pixel_offset, SEEK_SET) != 0)) {
		fb_gfx3_file_release_view(&view);
		return FB_GFX3_FAILED;
	}
	if (rle_compressed) {
		result = file_api_decode_rle(file, (uint32_t)signed_width,
			(uint32_t)signed_height, bits_per_pixel, &rle_indices);
		if (result != FB_GFX3_OK) {
			fb_gfx3_file_release_view(&view);
			return result;
		}
	} else {
		source_row = (unsigned char *)malloc(source_row_pitch);
		if (source_row == NULL) {
			fb_gfx3_file_release_view(&view);
			return FB_GFX3_OUT_OF_MEMORY;
		}
	}
	result = FB_GFX3_OK;
	for (y = 0; y < (uint32_t)signed_height; ++y) {
		uint32_t destination_y;

		if (rle_compressed) {
			source_row = rle_indices + ((size_t)y * (uint32_t)signed_width);
		} else if (fread(source_row, 1, source_row_pitch, file) !=
		    source_row_pitch) {
			result = FB_GFX3_FAILED;
			break;
		}
		destination_y = top_down ? y :
			((uint32_t)signed_height - 1u - y);
		if (destination_y >= copy_height)
			continue;
		destination_row = view.pixels +
			((size_t)destination_y * view.pitch);
		for (x = 0; x < copy_width; ++x) {
			index = 0;
			if (bits_per_pixel <= 8) {
				if (rle_compressed) {
					index = source_row[x];
				} else if (bits_per_pixel == 8) {
					index = source_row[x];
				} else if (bits_per_pixel == 4) {
					index = (uint8_t)((source_row[x / 2u] >>
						((x & 1u) ? 0u : 4u)) & 0x0Fu);
				} else {
					index = (uint8_t)((source_row[x / 8u] >>
						(7u - (x & 7u))) & 1u);
				}
				color = source_palette[index];
			} else if (compression == FB_GFX3_BMP_COMPRESSION_BITFIELDS) {
				uint32_t source_value = 0u;
				uint32_t red;
				uint32_t green;
				uint32_t blue;
				uint32_t alpha;

				if (bits_per_pixel == 16) {
					uint16_t source16;

					memcpy(&source16, source_row + ((size_t)x * 2u),
						sizeof(source16));
					source_value = source16;
				} else {
					memcpy(&source_value, source_row + ((size_t)x * 4u),
						sizeof(source_value));
				}
				red = file_api_expand_masked(source_value, masks[0],
					shifts[0], maximums[0]);
				green = file_api_expand_masked(source_value, masks[1],
					shifts[1], maximums[1]);
				blue = file_api_expand_masked(source_value, masks[2],
					shifts[2], maximums[2]);
				alpha = file_api_expand_masked(source_value, masks[3],
					shifts[3], maximums[3]);
				color = (alpha << 24) | (red << 16) | (green << 8) | blue;
			} else if (bits_per_pixel == 16) {
				uint16_t source16;
				uint32_t red;
				uint32_t green;
				uint32_t blue;

				memcpy(&source16, source_row + ((size_t)x * 2u),
					sizeof(source16));
				red = ((source16 >> 10) & 31u) * 255u / 31u;
				green = ((source16 >> 5) & 31u) * 255u / 31u;
				blue = (source16 & 31u) * 255u / 31u;
				color = 0xFF000000u | (red << 16) |
					(green << 8) | blue;
			} else {
				const unsigned char *source_pixel = source_row +
					((size_t)x * (bits_per_pixel / 8u));

				color = 0xFF000000u |
					((uint32_t)source_pixel[2] << 16) |
					((uint32_t)source_pixel[1] << 8) |
					source_pixel[0];
				if (bits_per_pixel == 32)
					color = (color & 0x00FFFFFFu) |
						((uint32_t)source_pixel[3] << 24);
			}
			file_api_store_color(destination_row +
				((size_t)x * view.bytes_per_pixel),
				view.bytes_per_pixel, color, index);
		}
	}
	if (!rle_compressed)
		free(source_row);
	free(rle_indices);
	if ((result == FB_GFX3_OK) && (output_view == NULL))
		result = fb_gfx3_file_commit_screen_locked(&view, copy_width,
			copy_height);
	if ((result == FB_GFX3_OK) && (output_view == NULL) &&
	    (destination != NULL)) {
		FB_GFX3_IMAGE_VIEW destination_view;

		/*
			The shared file view helper already accepted this as a gfxlib3
			image.
			Advance its cache generation so a later PUT cannot reuse the
			pre-BLOAD GPU texture.
		*/
		if (fb_gfx3_image_parse(destination, &destination_view) == FB_GFX3_OK)
			fb_gfx3_image_cache_metadata_touch(&destination_view);
	}
	if ((result == FB_GFX3_OK) && (output_view != NULL)) {
		*output_view = view;
		memset(&view, 0, sizeof(view));
	}
	fb_gfx3_file_release_view(&view);
	return result;
}

int fb_gfx3_file_load_bitmap_pixels_locked(FBSTRING *filename,
	uint32_t depth, unsigned char **pixels, uint32_t *width,
	uint32_t *height, uint32_t *pitch)
{
	FB_GFX3_FILE_VIEW view;
	FILE *file = NULL;
	char path[FB_GFX3_FILE_PATH_MAX];
	uint8_t identifier;
	int result = FB_GFX3_INVALID;

	memset(&view, 0, sizeof(view));
	if (pixels != NULL)
		*pixels = NULL;
	if (width != NULL)
		*width = 0u;
	if (height != NULL)
		*height = 0u;
	if (pitch != NULL)
		*pitch = 0u;
	if ((filename == NULL) || (filename->data == NULL) || (pixels == NULL) ||
	    (width == NULL) || (height == NULL) || (pitch == NULL) ||
	    !((depth == 8u) || (depth == 16u) || (depth == 32u)))
		goto done;
	if (!file_api_copy_path(path, sizeof(path), filename))
		goto done;
	fb_hConvertPath(path);
	file = fb_hOpenFile(path, "rb");
	if (file == NULL) {
		result = FB_GFX3_FAILED;
		goto done;
	}
	result = FB_GFX3_FAILED;
	if (file_api_read_u8(file, &identifier) &&
	    (fseek(file, 0, SEEK_SET) == 0)) {
		if (identifier == 'B') {
			result = file_api_load_bmp(file, NULL, NULL, depth, &view);
		} else if (identifier == 0x89u) {
			result = fb_gfx3_png_load_pixels_locked(file, depth,
				&view.allocation, &view.width, &view.height,
				&view.pitch);
			if (result == FB_GFX3_OK) {
				view.pixels = view.allocation;
				view.bytes_per_pixel =
					file_api_bytes_per_pixel(depth);
			}
		}
	}
	if ((fclose(file) != 0) && (result == FB_GFX3_OK))
		result = FB_GFX3_FAILED;
	file = NULL;
	if (result == FB_GFX3_OK) {
		*pixels = view.allocation;
		*width = view.width;
		*height = view.height;
		*pitch = view.pitch;
		view.allocation = NULL;
		view.pixels = NULL;
	}

done:
	if (file != NULL)
		fclose(file);
	fb_gfx3_file_release_view(&view);
	if (filename != NULL)
		fb_hStrDelTemp(filename);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Public BSAVE/BLOAD ABI                                                    */
/* ------------------------------------------------------------------------- */

static int file_api_has_extension(const char *path, const char expected[4])
{
	const char *extension;
	char actual[4];
	uint32_t i;

	if ((path == NULL) || (expected == NULL))
		return FALSE;
	extension = strrchr(path, '.');
	if ((extension == NULL) || (strlen(extension) != 4u))
		return FALSE;
	for (i = 0; i < 3u; ++i) {
		actual[i] = extension[i + 1u];
		if ((actual[i] >= 'A') && (actual[i] <= 'Z'))
			actual[i] = (char)(actual[i] + ('a' - 'A'));
	}
	actual[3] = '\0';
	return memcmp(actual, expected, sizeof(actual)) == 0;
}

FBCALL int fb_GfxBsaveEx(FBSTRING *filename, void *source,
	unsigned int size, void *palette, int bits_per_pixel)
{
	FB_GFX3_FILE_VIEW view;
	FILE *file;
	char path[FB_GFX3_FILE_PATH_MAX];
	int result;

	memset(&view, 0, sizeof(view));
	if ((filename == NULL) || (filename->data == NULL))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	FB_GRAPHICS_LOCK();
	if (!file_api_copy_path(path, sizeof(path), filename)) {
		fb_hStrDelTemp(filename);
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	fb_hConvertPath(path);
	file = fb_hOpenFile(path, "wb");
	if (file == NULL) {
		fb_hStrDelTemp(filename);
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
	}
	if (file_api_has_extension(path, "bmp")) {
		result = fb_gfx3_file_prepare_view_locked(source, TRUE, &view);
		if (result == FB_GFX3_OK)
			result = file_api_save_bmp(file, &view, palette,
				bits_per_pixel);
		fb_gfx3_file_release_view(&view);
	} else if (file_api_has_extension(path, "png")) {
		result = fb_gfx3_file_prepare_view_locked(source, TRUE, &view);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_png_save_locked(file, &view, palette,
				bits_per_pixel);
		fb_gfx3_file_release_view(&view);
	} else if ((source != NULL) && (size == 0)) {
		result = FB_GFX3_INVALID;
	} else {
		result = FB_GFX3_OK;
		if (!file_api_write_u8(file, 0xFEu) ||
		    !file_api_write_u32(file, size))
			result = FB_GFX3_FAILED;
		if ((result == FB_GFX3_OK) && (source != NULL) &&
		    (fwrite(source, 1, size, file) != size))
			result = FB_GFX3_FAILED;
		if ((result == FB_GFX3_OK) && (source == NULL)) {
			result = fb_gfx3_file_prepare_view_locked(NULL, TRUE, &view);
			if (result == FB_GFX3_OK) {
				size_t available =
					(size_t)view.pitch * view.height;
				size_t write_size = (size < available) ?
					(size_t)size : available;

				if (fwrite(view.pixels, 1, write_size, file) !=
				    write_size)
					result = FB_GFX3_FAILED;
			}
			fb_gfx3_file_release_view(&view);
		}
	}
	if ((fclose(file) != 0) && (result == FB_GFX3_OK))
		result = FB_GFX3_FAILED;
	fb_hStrDelTemp(filename);
	FB_GRAPHICS_UNLOCK();
	if (result == FB_GFX3_OK)
		return fb_ErrorSetNum(FB_RTERROR_OK);
	if (result == FB_GFX3_OUT_OF_MEMORY)
		return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
	if (result == FB_GFX3_INVALID)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	return fb_ErrorSetNum(FB_RTERROR_FILEIO);
}

FBCALL int fb_GfxBsave(FBSTRING *filename, void *source,
	unsigned int size, void *palette)
{
	return fb_GfxBsaveEx(filename, source, size, palette, 0);
}

static int file_api_load_raw(FILE *file, void *destination,
	uint32_t byte_count)
{
	FB_GFX3_FILE_VIEW view;
	size_t available;
	size_t read_size;
	int result;

	if (destination != NULL) {
		if ((byte_count != 0) &&
		    (fread(destination, 1, byte_count, file) != byte_count))
			return FB_GFX3_FAILED;
		return FB_GFX3_OK;
	}
	result = fb_gfx3_file_prepare_view_locked(NULL, TRUE, &view);
	if (result != FB_GFX3_OK)
		return result;
	available = (size_t)view.pitch * view.height;
	read_size = (byte_count < available) ? byte_count : available;
	if ((read_size != 0) &&
	    (fread(view.pixels, 1, read_size, file) != read_size))
		result = FB_GFX3_FAILED;
	if (result == FB_GFX3_OK)
		result = fb_gfx3_file_commit_screen_locked(&view, view.width,
			view.height);
	fb_gfx3_file_release_view(&view);
	return result;
}

static int file_api_bload(FBSTRING *filename, void *destination,
	void *palette)
{
	FILE *file;
	char path[FB_GFX3_FILE_PATH_MAX];
	uint8_t identifier;
	uint16_t qb_segment;
	uint16_t qb_offset;
	uint16_t qb_size;
	uint32_t byte_count;
	int result;

	if ((filename == NULL) || (filename->data == NULL))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	FB_GRAPHICS_LOCK();
	if (!file_api_copy_path(path, sizeof(path), filename)) {
		fb_hStrDelTemp(filename);
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	fb_hConvertPath(path);
	file = fb_hOpenFile(path, "rb");
	if (file == NULL) {
		fb_hStrDelTemp(filename);
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
	}
	result = FB_GFX3_FAILED;
	if (file_api_read_u8(file, &identifier)) {
		if (identifier == 'B') {
			if (fseek(file, 0, SEEK_SET) == 0)
				result = file_api_load_bmp(file, destination, palette, 0u, NULL);
		} else if (identifier == 0x89u) {
			if (fseek(file, 0, SEEK_SET) == 0)
				result = fb_gfx3_png_load_locked(file, destination,
					palette);
		} else if (identifier == 0xFEu) {
			if (file_api_read_u32(file, &byte_count))
				result = file_api_load_raw(file, destination,
					byte_count);
		} else if (identifier == 0xFDu) {
			if (file_api_read_u16(file, &qb_segment) &&
			    file_api_read_u16(file, &qb_offset) &&
			    file_api_read_u16(file, &qb_size))
				result = file_api_load_raw(file, destination, qb_size);
		}
	}
	if ((fclose(file) != 0) && (result == FB_GFX3_OK))
		result = FB_GFX3_FAILED;
	fb_hStrDelTemp(filename);
	FB_GRAPHICS_UNLOCK();
	if (result == FB_GFX3_OK)
		return fb_ErrorSetNum(FB_RTERROR_OK);
	if (result == FB_GFX3_OUT_OF_MEMORY)
		return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
	if (result == FB_GFX3_INVALID)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	return fb_ErrorSetNum(FB_RTERROR_FILEIO);
}

FBCALL int fb_GfxBload(FBSTRING *filename, void *destination,
	void *palette)
{
	return file_api_bload(filename, destination, palette);
}

FBCALL int fb_GfxBloadQB(FBSTRING *filename, void *destination,
	void *palette)
{
	return file_api_bload(filename, destination, palette);
}

/* end of gfx3_file_api.c */
