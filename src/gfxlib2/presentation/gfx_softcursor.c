/*
 * FreeBASIC gfxlib2: gfx_softcursor.c
 * Purpose: Draw and restore the software mouse cursor.
 * Responsibilities: presentation helpers and driver synchronization.
 * This file intentionally does NOT contain BASIC coordinate interpretation or input dispatch.
 */

/* Software cursor helper routines */

#include "../fb_gfx.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define CURSOR_W	13
#define CURSOR_H	22
/* Indexed graphics modes expose the standard 256-entry palette. */
#define CURSOR_PALETTE_SIZE 256

#define BIT_ENCODE(p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12) \
	((p0)|(p1<<2)|(p2<<4)|(p3<<6)|(p4<<8)|(p5<<10)|(p6<<12)|(p7<<14)|(p8<<16)|(p9<<18)|(p10<<20)|(p11<<22)|(p12<<24))

/* The DOS driver uses these boundary symbols to lock cursor data in memory. */
extern char fb_hSoftCursor_data_start;
char fb_hSoftCursor_data_start;

static const unsigned int cursor_data[] = {
	BIT_ENCODE(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0),
	BIT_ENCODE(2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0),
	BIT_ENCODE(2, 1, 1, 1, 2, 1, 1, 2, 3, 3, 3, 3, 3),
	BIT_ENCODE(2, 1, 1, 2, 2, 1, 1, 2, 3, 0, 0, 0, 0),
	BIT_ENCODE(2, 1, 2, 3, 3, 2, 1, 1, 2, 0, 0, 0, 0),
	BIT_ENCODE(2, 2, 3, 0, 0, 2, 1, 1, 2, 3, 0, 0, 0),
	BIT_ENCODE(2, 3, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0),
	BIT_ENCODE(0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 3, 0, 0),
	BIT_ENCODE(0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0),
	BIT_ENCODE(0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 3, 0),
	BIT_ENCODE(0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 3, 3, 0),
	BIT_ENCODE(0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0)
};

static unsigned char *cursor_area;
static size_t cursor_area_size;
static unsigned int white, black;

extern char fb_hSoftCursor_data_end;
char fb_hSoftCursor_data_end;

void fb_hSoftCursor_code_start(void) { }

/* Clip before calculating offsets so partially visible cursors stay in bounds. */
static int cursor_clip(int x, int y, int *left, int *top, int *right, int *bottom)
{
	int64_t right_edge, bottom_edge;
	size_t bytes_per_pixel, row_bytes;

	if (!fb_gfx || !fb_gfx->framebuffer || !left || !top || !right || !bottom ||
	    (fb_gfx->w <= 0) || (fb_gfx->h <= 0) ||
	    (fb_gfx->bpp < 1) || (fb_gfx->bpp > (int)sizeof(unsigned int)) ||
	    (fb_gfx->pitch <= 0))
		return 0;

	bytes_per_pixel = (size_t)fb_gfx->bpp;
	if ((size_t)fb_gfx->w > (SIZE_MAX / bytes_per_pixel) ||
	    (size_t)fb_gfx->h > (SIZE_MAX / (size_t)fb_gfx->pitch))
		return 0;
	row_bytes = (size_t)fb_gfx->w * bytes_per_pixel;
	if ((size_t)fb_gfx->pitch < row_bytes)
		return 0;

	*left = (x < 0) ? 0 : x;
	*top = (y < 0) ? 0 : y;
	right_edge = (int64_t)x + CURSOR_W;
	bottom_edge = (int64_t)y + CURSOR_H;
	if ((right_edge <= 0) || (bottom_edge <= 0))
		return 0;
	if (right_edge > fb_gfx->w)
		right_edge = fb_gfx->w;
	if (bottom_edge > fb_gfx->h)
		bottom_edge = fb_gfx->h;
	if (((int64_t)*left >= right_edge) || ((int64_t)*top >= bottom_edge))
		return 0;

	*right = (int)right_edge;
	*bottom = (int)bottom_edge;
	return 1;
}

static void copy_cursor_area(int x, int y, int from_area)
{
	int left, top, right, bottom;
	size_t bytes_per_pixel, cursor_pitch, cursor_offset, cursor_size;
	size_t framebuffer_pitch, framebuffer_offset, framebuffer_size;
	size_t bytes_per_row, rows, last_row_offset, row;

	if (!cursor_area || !cursor_clip(x, y, &left, &top, &right, &bottom))
		return;

	bytes_per_pixel = (size_t)fb_gfx->bpp;
	cursor_pitch = (size_t)CURSOR_W * bytes_per_pixel;
	cursor_size = cursor_pitch * CURSOR_H;
	if (cursor_area_size != cursor_size)
		return;
	bytes_per_row = (size_t)(right - left) * bytes_per_pixel;
	rows = (size_t)(bottom - top);
	cursor_offset = (size_t)((int64_t)top - y) * cursor_pitch +
		(size_t)((int64_t)left - x) * bytes_per_pixel;
	if ((cursor_offset > cursor_size) || (rows == 0) ||
	    ((rows - 1) > (SIZE_MAX / cursor_pitch)))
		return;
	last_row_offset = cursor_offset + ((rows - 1) * cursor_pitch);
	if ((last_row_offset > cursor_size) || (bytes_per_row > cursor_size - last_row_offset))
		return;

	framebuffer_pitch = (size_t)fb_gfx->pitch;
	framebuffer_size = (size_t)fb_gfx->h * framebuffer_pitch;
	framebuffer_offset = (size_t)top * framebuffer_pitch +
		(size_t)left * bytes_per_pixel;
	if ((bytes_per_row > framebuffer_pitch) ||
	    ((size_t)left * bytes_per_pixel > framebuffer_pitch - bytes_per_row) ||
	    (framebuffer_offset > framebuffer_size) ||
	    ((rows - 1) > (SIZE_MAX / framebuffer_pitch)))
		return;
	last_row_offset = framebuffer_offset + ((rows - 1) * framebuffer_pitch);
	if ((last_row_offset > framebuffer_size) ||
	    (bytes_per_row > framebuffer_size - last_row_offset))
		return;

	for (row = 0; row < rows; row++) {
		size_t area_row = cursor_offset + (row * cursor_pitch);
		size_t screen_row = framebuffer_offset + (row * framebuffer_pitch);
		if (from_area) {
			fb_hMemCpy(fb_gfx->framebuffer + screen_row,
				   cursor_area + area_row, bytes_per_row);
		} else {
			fb_hMemCpy(cursor_area + area_row,
				   fb_gfx->framebuffer + screen_row, bytes_per_row);
		}
	}
}

int fb_hColorDistance(int index, int r, int g, int b)
{
	unsigned int color;
	int red, green, blue;
	int red_distance, green_distance, blue_distance;

	if (!fb_gfx || !fb_gfx->device_palette ||
	    (index < 0) || (index >= CURSOR_PALETTE_SIZE) ||
	    (r < 0) || (r > 255) || (g < 0) || (g > 255) || (b < 0) || (b > 255))
		return INT_MAX;

	color = fb_gfx->device_palette[(size_t)index];
	red = (int)(color & 0xFFu);
	green = (int)((color >> 8) & 0xFFu);
	blue = (int)((color >> 16) & 0xFFu);
	red_distance = red - r;
	green_distance = green - g;
	blue_distance = blue - b;
	return (red_distance * red_distance) +
	       (green_distance * green_distance) +
	       (blue_distance * blue_distance);
}

int fb_hSoftCursorInit(void)
{
	size_t cursor_size;
	size_t row_bytes;
	unsigned char *new_cursor_area;

	if ((!fb_gfx) || (!fb_gfx->framebuffer) ||
	    (fb_gfx->w <= 0) || (fb_gfx->h <= 0) ||
	    (fb_gfx->bpp < 1) || (fb_gfx->bpp > (int)sizeof(unsigned int)) ||
	    (fb_gfx->pitch <= 0))
		return -1;

	if ((size_t)fb_gfx->w > (SIZE_MAX / (size_t)fb_gfx->bpp) ||
	    (size_t)fb_gfx->h > (SIZE_MAX / (size_t)fb_gfx->pitch))
		return -1;
	row_bytes = (size_t)fb_gfx->w * (size_t)fb_gfx->bpp;
	if ((size_t)fb_gfx->pitch < row_bytes)
		return -1;
	cursor_size = (size_t)CURSOR_W * CURSOR_H * (size_t)fb_gfx->bpp;
	new_cursor_area = (unsigned char *)calloc(1, cursor_size);
	if (!new_cursor_area)
		return -1;
	free(cursor_area);
	cursor_area = new_cursor_area;
	cursor_area_size = cursor_size;

	if (fb_gfx->bpp == 1) {
		white = 15;
		black = 0;
	}
	else {
		white = fb_hFixColor(fb_gfx->bpp, 0xFFFFFF);
		black = fb_hFixColor(fb_gfx->bpp, 0x000000);
	}

	return 0;
}

void fb_hSoftCursorExit(void)
{
	if (!cursor_area)
		return;

	free(cursor_area);
	cursor_area = NULL;
	cursor_area_size = 0;
}

void fb_hSoftCursorPut(int x, int y)
{
	unsigned char *d, *dest;
	int left, top, right, bottom, source_x, source_y;
	int w, h, px, py, count;
	unsigned int pixel;
	unsigned int data;
	size_t destination_offset, bytes_per_pixel;

	if (!cursor_area || !cursor_clip(x, y, &left, &top, &right, &bottom))
		return;
	bytes_per_pixel = (size_t)fb_gfx->bpp;
	if (cursor_area_size != ((size_t)CURSOR_W * CURSOR_H * bytes_per_pixel))
		return;
	copy_cursor_area(x, y, FALSE);

	source_x = (int)((int64_t)left - x);
	source_y = (int)((int64_t)top - y);
	w = right - left;
	h = bottom - top;
	destination_offset = (size_t)top * (size_t)fb_gfx->pitch +
		(size_t)left * bytes_per_pixel;
	dest = fb_gfx->framebuffer + destination_offset;
	for (py = 0; py < h; py++) {
		d = dest;
		data = cursor_data[source_y + py] >> ((unsigned int)source_x * 2u);
		for (px = 0; px < w;) {
			pixel = data & 0x3u;
			for (count = 0; (px < w) && ((data & 0x3u) == pixel); px++, data >>= 2)
				count++;
			if (pixel == 0x3u) {
				if (fb_gfx->bpp == 4)
					fb_hPixelSetAlpha4(d, 0x40000000, (size_t)count);
			}
			else {
				if (pixel != 0u)
					fb_hPixelSet(d, (int)((pixel & 0x1u) ? white : black), (size_t)count);
			}
			d += (size_t)count * bytes_per_pixel;
		}
		if (fb_gfx->dirty)
			fb_gfx->dirty[(size_t)(top + py)] = TRUE;
		dest += (size_t)fb_gfx->pitch;
	}
}

void fb_hSoftCursorUnput(int x, int y)
{
	int left, top, right, bottom;

	if (!cursor_area || !cursor_clip(x, y, &left, &top, &right, &bottom))
		return;
	if (cursor_area_size != ((size_t)CURSOR_W * CURSOR_H * (size_t)fb_gfx->bpp))
		return;
	copy_cursor_area(x, y, TRUE);
	if (fb_gfx->dirty)
		fb_hMemSet(fb_gfx->dirty + (size_t)top, TRUE, (size_t)(bottom - top));
}

void fb_hSoftCursorPaletteChanged(void)
{
	int i, dist, min_wdist = 1000000, min_bdist = 1000000;

	if (!fb_gfx || !fb_gfx->device_palette || (fb_gfx->bpp != 1))
		return;
	for (i = 0; i < CURSOR_PALETTE_SIZE; i++) {
		dist = fb_hColorDistance(i, 255, 255, 255);
		if (dist < min_wdist) {
			min_wdist = dist;
			white = (unsigned int)i;
		}
		dist = fb_hColorDistance(i, 0, 0, 0);
		if (dist < min_bdist) {
			min_bdist = dist;
			black = (unsigned int)i;
		}
	}
}

void fb_hSoftCursor_code_end(void) { }

/* end of gfx_softcursor.c */
