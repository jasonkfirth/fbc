/*
    FreeBASIC DOS graphics: gfx_softcursor.c

    Draw and restore the software cursor and mark its published rows dirty.
    This complete target file is selected by basename precedence. Cursor
    storage stays locked for IRQ0 use until screen shutdown releases it.
    Input polling and video publication belong to the DOS driver.
*/

#include "../../fb_gfx.h"

#define CURSOR_W	13
#define CURSOR_H	22

#define BIT_ENCODE(p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12) \
	((p0)|(p1<<2)|(p2<<4)|(p3<<6)|(p4<<8)|(p5<<10)|(p6<<12)|(p7<<14)|(p8<<16)|(p9<<18)|(p10<<20)|(p11<<22)|(p12<<24))

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

char fb_hSoftCursor_data_end;

void fb_hSoftCursor_code_start(void) { }

static int get_cursor_bounds(int x, int y, size_t *row_bytes, size_t *rows,
	                         size_t *framebuffer_offset)
{
	size_t width, height, bytes_per_pixel, pitch, x_offset, y_offset;

	if (!fb_gfx || !fb_gfx->framebuffer || !row_bytes || !rows ||
	    !framebuffer_offset || x < 0 || y < 0 || x >= fb_gfx->w ||
	    y >= fb_gfx->h || fb_gfx->bpp < 1 || fb_gfx->pitch < 1)
		return 0;

	width = (size_t)(fb_gfx->w - x);
	if (width > (size_t)CURSOR_W)
		width = (size_t)CURSOR_W;
	height = (size_t)(fb_gfx->h - y);
	if (height > (size_t)CURSOR_H)
		height = (size_t)CURSOR_H;
	bytes_per_pixel = (size_t)fb_gfx->bpp;
	pitch = (size_t)fb_gfx->pitch;
	if ((width > (size_t)-1 / bytes_per_pixel) ||
	    ((size_t)x > (size_t)-1 / bytes_per_pixel))
		return 0;
	*row_bytes = width * bytes_per_pixel;
	x_offset = (size_t)x * bytes_per_pixel;
	if ((x_offset > pitch) || (*row_bytes > pitch - x_offset) ||
	    ((size_t)y > (size_t)-1 / pitch))
		return 0;
	y_offset = (size_t)y * pitch;
	if (x_offset > (size_t)-1 - y_offset)
		return 0;
	*framebuffer_offset = y_offset + x_offset;
	*rows = height;
	return 1;
}

static void copy_cursor_area(size_t framebuffer_offset, size_t row_bytes,
	                         size_t rows, int from_area)
{
	unsigned char *source, *destination;
	size_t source_pitch, destination_pitch, row;

	if (from_area) {
		source = cursor_area;
		destination = fb_gfx->framebuffer + framebuffer_offset;
		source_pitch = row_bytes;
		destination_pitch = (size_t)fb_gfx->pitch;
	} else {
		source = fb_gfx->framebuffer + framebuffer_offset;
		destination = cursor_area;
		source_pitch = (size_t)fb_gfx->pitch;
		destination_pitch = row_bytes;
	}

	for (row = 0; row < rows; row++) {
		fb_hMemCpy(destination, source, row_bytes);
		source += source_pitch;
		destination += destination_pitch;
	}
}

int fb_hColorDistance(int index, int r, int g, int b)
{
	unsigned int color;
	int red, green, blue;

	if (!fb_gfx || !fb_gfx->device_palette || index < 0 || index >= 256)
		return 0;
	color = fb_gfx->device_palette[index];
	red = (int)(color & 0xFFU) - r;
	green = (int)((color >> 8) & 0xFFU) - g;
	blue = (int)((color >> 16) & 0xFFU) - b;
	return (red * red) + (green * green) + (blue * blue);
}

int fb_hSoftCursorInit(void)
{
	size_t cursor_size;

	if ((!fb_gfx) || (fb_gfx->bpp < 1) || (fb_gfx->bpp > (int)sizeof(unsigned int)))
		return -1;
	if (cursor_area)
		return 0;

	cursor_size = (size_t)CURSOR_W * CURSOR_H * (size_t)fb_gfx->bpp;
	cursor_area = calloc(1, cursor_size);
	if (!cursor_area)
		return -1;
	cursor_area_size = cursor_size;

	if (fb_dos_lock_data(cursor_area, cursor_size) != 0) {
		free(cursor_area);
		cursor_area = NULL;
		cursor_area_size = 0;
		return -1;
	}

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

	fb_dos_unlock_data(cursor_area, cursor_area_size);
	free(cursor_area);
	cursor_area = NULL;
	cursor_area_size = 0;
}

void fb_hSoftCursorPut(int x, int y)
{
	unsigned char *d, *dest;
	size_t row_bytes, rows, framebuffer_offset;
	int w, h, px, py, pixel, count;
	unsigned int data;
	const unsigned int *cursor;

	if (!cursor_area || !fb_gfx || !fb_gfx->dirty ||
	    !get_cursor_bounds(x, y, &row_bytes, &rows, &framebuffer_offset))
		return;
	copy_cursor_area(framebuffer_offset, row_bytes, rows, FALSE);

	w = (int)(row_bytes / (size_t)fb_gfx->bpp);
	h = (int)rows;
	dest = fb_gfx->framebuffer + framebuffer_offset;
	cursor = cursor_data;
	for (py = 0; py < h; py++) {
		d = dest;
		data = *cursor++;
		for (px = 0; px < w;) {
			pixel = (int)(data & 0x3U);
			for (count = 0; (px < w) && ((int)(data & 0x3U) == pixel); px++, data >>= 2)
				count++;
			if (pixel == 0x3) {
				if (fb_gfx->bpp == 4)
					fb_hPixelSetAlpha4(d, 0x40000000, (size_t)count);
			}
			else {
				if (pixel)
					fb_hPixelSet(d, (pixel & 0x1) ? (int)white : (int)black,
					             (size_t)count);
			}
			d += (size_t)count * (size_t)fb_gfx->bpp;
		}
		fb_gfx->dirty[y + py] = TRUE;
		dest += fb_gfx->pitch;
	}
}

/* Mark the rows that still contain a previously published software cursor.
 * This does not restore pixels: the foreground may have edited its background
 * since the last refresh. DOS calls it from the locked IRQ0 presentation path.
 */
void fb_hSoftCursorMarkDirty(int y)
{
	int rows;
	if (!fb_gfx || !fb_gfx->dirty || y < 0 || y >= fb_gfx->h)
		return;
	rows = fb_gfx->h - y;
	if (rows > CURSOR_H)
		rows = CURSOR_H;
	fb_hMemSet(fb_gfx->dirty + y, TRUE, (size_t)rows);
}

void fb_hSoftCursorUnput(int x, int y)
{
	size_t row_bytes, rows, framebuffer_offset;
	if (!cursor_area || !fb_gfx || !fb_gfx->dirty ||
	    !get_cursor_bounds(x, y, &row_bytes, &rows, &framebuffer_offset))
		return;
	copy_cursor_area(framebuffer_offset, row_bytes, rows, TRUE);
	fb_hMemSet(fb_gfx->dirty + y, TRUE, rows);
}

void fb_hSoftCursorPaletteChanged(void)
{
	int i, dist, min_wdist = 1000000, min_bdist = 1000000;

	if (fb_gfx->bpp > 1)
		return;
	for (i = 0; i < 256; i++) {
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
