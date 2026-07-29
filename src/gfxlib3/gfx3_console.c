/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_console.c

    Purpose:

        Implement the FreeBASIC graphical text console above GPU-resident
        screen pages.

    Responsibilities:

        - render canonical 8-pixel glyphs in 8- or 9-pixel console cells
		- scroll text regions with overlap-safe GPU surface blits
		- track characters and colors independently for every logical page
		- implement COLOR, CLS, WIDTH, LOCATE, cursor, size, and SCREEN reads
		- install the corresponding FreeBASIC runtime hook table entries

    This file intentionally does NOT contain:

        - native input event collection or line editing
		- scalable fonts, Unicode shaping, or platform text calls
		- screen mode creation or destruction
*/

#include "gfx3_api_internal.h"
#include "gfx3_backend_opengl.h"
#include "gfx3_backend_vulkan.h"
#include "gfx3_console.h"
#include "gfx3_data.h"
#include "gfx3_image.h"

typedef struct FB_GFX3_CONSOLE_CELL {
	uint32_t character;
	uint32_t foreground;
	uint32_t background;
} FB_GFX3_CONSOLE_CELL;

typedef struct FB_GFX3_CONSOLE_STATE {
	FB_GFX3_MODE *mode;
	FB_GFX3_CONSOLE_CELL *cells;
	uint32_t columns;
	uint32_t rows;
	uint32_t font_width;
	uint32_t font_height;
	const unsigned char *font;
	uint32_t cursor_x;
	uint32_t cursor_y;
	int initialized;
} FB_GFX3_CONSOLE_STATE;

typedef struct FB_GFX3_CONSOLE_PRINT_INFO {
	FB_GFX3_DRAW_STATE *state;
} FB_GFX3_CONSOLE_PRINT_INFO;

static FB_GFX3_CONSOLE_STATE console_state;

/*
	The runtime hook table uses the C calling convention on 32-bit Windows.
	These functions therefore intentionally omit FBCALL, which expands to
	__stdcall for public runtime entry points on that target. Keeping the hook
	implementations cdecl matches gfxlib2 and keeps the caller and callee in
	agreement about which side restores the stack.
*/
unsigned int fb_GfxColor(unsigned int foreground,
	unsigned int background, int flags);
void fb_GfxClear(int mode);
int fb_GfxWidth(int columns, int rows);
int fb_GfxLocateRaw(int row, int column, int cursor);
int fb_GfxGetX(void);
int fb_GfxGetY(void);
void fb_GfxPrintBuffer(const char *buffer, int mask);
void fb_GfxPrintBufferWstrEx(const FB_WCHAR *buffer, size_t length,
	int mask);
void fb_GfxPrintBufferWstr(const FB_WCHAR *buffer, int mask);
unsigned int fb_GfxReadXY(int column, int row, int color_flag);
int fb_GfxPageCopy(int from_page, int to_page);
int fb_GfxPageSet(int work_page, int visible_page);

/* ------------------------------------------------------------------------- */
/* Cell and target helpers                                                   */
/* ------------------------------------------------------------------------- */

static const unsigned char *console_font_for_height(uint32_t height)
{
	switch (height) {
	case 8u:
		return fb_gfx3_data_font_8x8();
	case 14u:
		return fb_gfx3_data_font_8x14();
	case 16u:
		return fb_gfx3_data_font_8x16();
	default:
		return NULL;
	}
}

static FB_GFX3_CONSOLE_CELL *console_page_cells(
	const FB_GFX3_DRAW_STATE *state)
{
	size_t page_size;

	if (!console_state.initialized || (state == NULL) ||
	    (state->mode != console_state.mode) ||
	    (state->work_page >= state->mode->page_count))
		return NULL;
	page_size = (size_t)console_state.columns * console_state.rows;
	return console_state.cells + ((size_t)state->work_page * page_size);
}

static FB_GFX3_SURFACE *console_page_surface(FB_GFX3_DRAW_STATE *state)
{
	if ((state == NULL) || (state->mode == NULL) ||
	    (state->work_page >= state->mode->page_count))
		return NULL;
	return &state->mode->pages[state->work_page];
}

/*
	Page copies are a graphics ABI operation, rather than merely a GPU blit.
	gfxlib2 copies the matching character-cell page as well, which lets a later
	SCREEN(row, column) observe exactly the text that was presented.  The caller
	holds the active mode mutex, so the static console allocation cannot be
	replaced while this copy is in progress.
*/
int fb_gfx3_console_page_copy_locked(FB_GFX3_MODE *mode,
	uint32_t source_page, uint32_t destination_page)
{
	size_t cells_per_page;
	size_t source_offset;
	size_t destination_offset;
	size_t copy_size;

	if (!console_state.initialized)
		return FB_GFX3_OK;
	if ((mode == NULL) || (console_state.mode != mode) ||
	    (console_state.cells == NULL) || (source_page >= mode->page_count) ||
	    (destination_page >= mode->page_count))
		return FB_GFX3_INVALID;
	if (source_page == destination_page)
		return FB_GFX3_OK;
	if ((fb_gfx3_size_multiply(console_state.columns, console_state.rows,
	     &cells_per_page) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(source_page, cells_per_page, &source_offset) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(destination_page, cells_per_page,
	     &destination_offset) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(cells_per_page, sizeof(console_state.cells[0]),
	     &copy_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	memcpy(console_state.cells + destination_offset,
		console_state.cells + source_offset, copy_size);
	return FB_GFX3_OK;
}

static uint32_t console_bytes_per_pixel(const FB_GFX3_MODE *mode)
{
	return (mode->depth + 7u) / 8u;
}

static uint32_t console_fix_color(const FB_GFX3_MODE *mode, uint32_t color)
{
	return fb_gfx3_image_fix_color(console_bytes_per_pixel(mode), color);
}

static void console_full_clip(const FB_GFX3_SURFACE *surface,
	FB_GFX3_RECT *clip)
{
	clip->x1 = 0;
	clip->y1 = 0;
	clip->x2 = (int32_t)surface->width - 1;
	clip->y2 = (int32_t)surface->height - 1;
}

/*
	OpenGL and Vulkan have native ordered glyph compute packets. GLES retains
	the proven point representation until it has an equally compact shader path.
*/
static int console_uses_gpu_glyphs(const FB_GFX3_SURFACE *surface)
{
	if ((surface == NULL) || (surface->context == NULL))
		return FALSE;
	return (surface->context->renderer.backend_vtable ==
		&__fb_gfx3_backend_opengl) ||
		(surface->context->renderer.backend_vtable ==
		 &__fb_gfx3_backend_vulkan);
}

static void console_clear_cells(FB_GFX3_DRAW_STATE *state, uint32_t x1,
	uint32_t y1, uint32_t x2, uint32_t y2)
{
	FB_GFX3_CONSOLE_CELL *cells = console_page_cells(state);
	uint32_t x;
	uint32_t y;

	if (cells == NULL)
		return;
	if (x2 > console_state.columns)
		x2 = console_state.columns;
	if (y2 > console_state.rows)
		y2 = console_state.rows;
	for (y = y1; y < y2; ++y) {
		for (x = x1; x < x2; ++x) {
			FB_GFX3_CONSOLE_CELL *cell = cells +
				((size_t)y * console_state.columns) + x;

			cell->character = 32;
			cell->foreground = state->foreground_color;
			cell->background = state->background_color;
		}
	}
}

static void console_invalidate_shadow(FB_GFX3_DRAW_STATE *state)
{
	if ((state != NULL) && (state->mode != NULL) &&
	    (state->mode->shadow_valid != NULL) &&
	    (state->work_page < state->mode->page_count))
		state->mode->shadow_valid[state->work_page] = FALSE;
	/* Console writes may touch a sparse glyph-shaped region. The POINT cache
	   holds only one coordinate, so a full-page invalidation is safer than
	   duplicating the console coverage calculation here. */
	if ((state != NULL) && (state->mode != NULL) &&
	    (state->mode->point_cache != NULL) &&
	    (state->work_page < state->mode->page_count))
		state->mode->point_cache[state->work_page].valid = FALSE;
}

/* Caller holds FB_GRAPHICS_LOCK(). */
static int console_set_layout(FB_GFX3_MODE *mode, uint32_t font_width,
	uint32_t font_height, uint32_t columns, uint32_t rows,
	uint32_t foreground, uint32_t background)
{
	FB_GFX3_CONSOLE_CELL *cells;
	const unsigned char *font;
	size_t cells_per_page;
	size_t cell_count;
	size_t allocation_size;
	size_t index;

	if ((mode == NULL) || (mode->width < font_width) ||
	    !((font_width == 8u) || (font_width == 9u)) ||
	    (font_height == 0u) || (mode->height < font_height) ||
	    (mode->page_count == 0u))
		return FB_GFX3_INVALID;
	font = console_font_for_height(font_height);
	if (font == NULL)
		return FB_GFX3_INVALID;
	if (columns == 0u)
		columns = mode->width / font_width;
	if (rows == 0u)
		rows = mode->height / font_height;
	if ((columns == 0u) || (rows == 0u) ||
	    ((uint64_t)columns * font_width > mode->width) ||
	    ((uint64_t)(rows - 1u) * font_height >= mode->height) ||
	    (fb_gfx3_size_multiply(columns, rows, &cells_per_page) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(cells_per_page, mode->page_count, &cell_count) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(cell_count, sizeof(cells[0]),
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	cells = (FB_GFX3_CONSOLE_CELL *)malloc(allocation_size);
	if (cells == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (index = 0; index < cell_count; ++index) {
		cells[index].character = 32u;
		cells[index].foreground = foreground;
		cells[index].background = background;
	}
	free(console_state.cells);
	console_state.cells = cells;
	console_state.columns = columns;
	console_state.rows = rows;
	console_state.font_width = font_width;
	console_state.font_height = font_height;
	console_state.font = font;
	console_state.cursor_x = 0u;
	console_state.cursor_y = 0u;
	mode->console_font_width = font_width;
	mode->console_font_height = font_height;
	mode->console_rows = rows;
	return FB_GFX3_OK;
}

/*
	Resize the character metadata without redrawing the GPU page.  The mode
	resize has already copied the overlapping pixel rectangle, so copying the
	matching top-left cells keeps SCREEN(row, column) consistent with the image
	while newly exposed cells describe the black area created by the renderer.
*/
int fb_gfx3_console_resize_locked(FB_GFX3_MODE *mode,
	uint32_t foreground, uint32_t background)
{
	FB_GFX3_CONSOLE_CELL *cells;
	uint32_t columns;
	uint32_t rows;
	uint32_t copy_columns;
	uint32_t copy_rows;
	size_t cells_per_page;
	size_t cell_count;
	size_t allocation_size;
	size_t index;
	uint32_t page;
	uint32_t row;

	if (!console_state.initialized || (console_state.mode != mode))
		return FB_GFX3_OK;
	if ((mode == NULL) || (mode->width < console_state.font_width) ||
	    (mode->height < console_state.font_height) ||
	    (mode->page_count == 0u))
		return FB_GFX3_INVALID;
	columns = mode->width / console_state.font_width;
	rows = mode->height / console_state.font_height;
	if ((columns == 0u) || (rows == 0u) ||
	    (fb_gfx3_size_multiply(columns, rows, &cells_per_page) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(cells_per_page, mode->page_count,
	     &cell_count) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(cell_count, sizeof(cells[0]),
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	cells = (FB_GFX3_CONSOLE_CELL *)malloc(allocation_size);
	if (cells == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (index = 0; index < cell_count; ++index) {
		cells[index].character = 32u;
		cells[index].foreground = foreground;
		cells[index].background = background;
	}
	copy_columns = MIN(columns, console_state.columns);
	copy_rows = MIN(rows, console_state.rows);
	for (page = 0; page < mode->page_count; ++page) {
		for (row = 0; row < copy_rows; ++row) {
			memcpy(cells + ((size_t)page * cells_per_page) +
				((size_t)row * columns),
				console_state.cells +
				((size_t)page * console_state.columns * console_state.rows) +
				((size_t)row * console_state.columns),
				(size_t)copy_columns * sizeof(cells[0]));
		}
	}
	free(console_state.cells);
	console_state.cells = cells;
	console_state.columns = columns;
	console_state.rows = rows;
	mode->console_rows = rows;
	if (console_state.cursor_x >= columns)
		console_state.cursor_x = columns - 1u;
	if (console_state.cursor_y >= rows)
		console_state.cursor_y = rows - 1u;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Console TTY callbacks                                                     */
/* ------------------------------------------------------------------------- */

static int console_write_glyphs(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const unsigned char *font,
	const unsigned char *text, size_t length, int32_t start_x,
	int32_t start_y, uint32_t foreground, uint32_t background)
{
	FB_GFX3_GLYPH local_glyphs[64];
	FB_GFX3_GLYPH *glyphs;
	size_t allocation_size;
	size_t character;
	int result;

	if ((surface == NULL) || (clip == NULL) || (font == NULL) ||
	    (text == NULL) || (length == 0u) || (length > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(length, sizeof(glyphs[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (length <= (sizeof(local_glyphs) / sizeof(local_glyphs[0]))) {
		glyphs = local_glyphs;
		memset(glyphs, 0, allocation_size);
	} else {
		glyphs = (FB_GFX3_GLYPH *)calloc(1, allocation_size);
		if (glyphs == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
	}
	for (character = 0u; character < length; ++character) {
		const unsigned char *source = font + ((size_t)text[character] *
			console_state.font_height);
		uint32_t row;

		glyphs[character].x = start_x +
			(int32_t)(character * console_state.font_width);
		glyphs[character].y = start_y;
		glyphs[character].foreground = foreground;
		glyphs[character].background = background;
		glyphs[character].width = console_state.font_width;
		glyphs[character].height = console_state.font_height;
		glyphs[character].flags = FB_GFX3_GLYPH_BACKGROUND;
		for (row = 0u; row < console_state.font_height; ++row)
			glyphs[character].row[row] = source[row];
	}
	result = fb_gfx3_surface_glyphs(surface, clip, glyphs, (uint32_t)length);
	if (glyphs != local_glyphs)
		free(glyphs);
	return result;
}

static int console_write(fb_ConHooks *hooks, const void *buffer,
	size_t length)
{
	FB_GFX3_CONSOLE_PRINT_INFO *info;
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	FB_GFX3_CONSOLE_CELL *cells;
	FB_GFX3_POINT *points = NULL;
	FB_GFX3_RECT clip;
	const unsigned char *text = (const unsigned char *)buffer;
	const unsigned char *font = console_state.font;
	size_t points_per_character;
	size_t maximum_points;
	size_t allocation_size;
	size_t character;
	uint32_t point_count = 0;
	int pixel_x;
	int pixel_y;
	int64_t start_x;
	int64_t end_x;
	int64_t start_y;
	int result = FB_GFX3_OK;
	int gpu_glyphs;

	if ((hooks == NULL) || (buffer == NULL) || (length == 0) ||
	    (font == NULL) || (console_state.font_width == 0u) ||
	    (console_state.font_height == 0u))
		return TRUE;
	info = (FB_GFX3_CONSOLE_PRINT_INFO *)hooks->Opaque;
	state = (info == NULL) ? NULL : info->state;
	surface = console_page_surface(state);
	cells = console_page_cells(state);
	if ((surface == NULL) || (cells == NULL) || (hooks->Coord.X < 0) ||
	    (hooks->Coord.Y < 0))
		return FALSE;
	start_x = (int64_t)hooks->Coord.X * console_state.font_width;
	start_y = (int64_t)hooks->Coord.Y * console_state.font_height;
	end_x = start_x + ((int64_t)length * console_state.font_width) - 1;
	if ((start_x < 0) || (start_y < 0) || (end_x > INT32_MAX) ||
	    (start_y + (int64_t)console_state.font_height - 1 > INT32_MAX))
		return FALSE;
	console_full_clip(surface, &clip);
	/*
		The OpenGL glyph packet chooses foreground or background from the row
		mask on shader units. Other backends keep the ordered background rectangle
		and foreground point batch.
	*/
	gpu_glyphs = console_uses_gpu_glyphs(surface);
	if (gpu_glyphs) {
		result = console_write_glyphs(surface, &clip, font, text, length,
			(int32_t)start_x, (int32_t)start_y,
			state->foreground_color, state->background_color);
		if (result != FB_GFX3_OK)
			return FALSE;
		for (character = 0; character < length; ++character) {
			if (((uint32_t)hooks->Coord.X + character <
			     console_state.columns) &&
			    ((uint32_t)hooks->Coord.Y < console_state.rows)) {
				FB_GFX3_CONSOLE_CELL *cell = cells +
					((size_t)(uint32_t)hooks->Coord.Y *
					 console_state.columns) +
					(uint32_t)hooks->Coord.X + character;

				cell->character = text[character];
				cell->foreground = state->foreground_color;
				cell->background = state->background_color;
			}
		}
		console_invalidate_shadow(state);
		return TRUE;
	}
	result = fb_gfx3_surface_rectangle(surface, &clip, (int)start_x,
		(int)start_y, (int)end_x,
		(int)(start_y + (int64_t)console_state.font_height - 1),
		state->background_color, 0xFFFFu, TRUE, 0);
	if (result != FB_GFX3_OK)
		return FALSE;
	if ((fb_gfx3_size_multiply(console_state.font_width,
	     console_state.font_height, &points_per_character) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(length, points_per_character,
	     &maximum_points) != FB_GFX3_OK) ||
	    (maximum_points == 0u) ||
	    (maximum_points > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(maximum_points, sizeof(points[0]),
	     &allocation_size) != FB_GFX3_OK) ||
	    (allocation_size == 0u))
		return FALSE;
	points = (FB_GFX3_POINT *)malloc(allocation_size);
	if (points == NULL)
		return FALSE;
	for (character = 0; character < length; ++character) {
		const unsigned char *glyph = font + ((size_t)text[character] *
			console_state.font_height);

		for (pixel_y = 0; pixel_y < (int)console_state.font_height; ++pixel_y) {
			for (pixel_x = 0; pixel_x < (int)console_state.font_width; ++pixel_x) {
				if ((glyph[pixel_y] & (1u << pixel_x)) == 0)
					continue;
				points[point_count].x = (int32_t)(start_x +
					((int64_t)character * console_state.font_width) + pixel_x);
				points[point_count].y = (int32_t)(start_y + pixel_y);
				points[point_count].color = state->foreground_color;
				points[point_count].flags = 0;
				point_count++;
			}
		}
		if (((uint32_t)hooks->Coord.X + character < console_state.columns) &&
		    ((uint32_t)hooks->Coord.Y < console_state.rows)) {
			FB_GFX3_CONSOLE_CELL *cell = cells +
				((size_t)(uint32_t)hooks->Coord.Y * console_state.columns) +
				(uint32_t)hooks->Coord.X + character;

			cell->character = text[character];
			cell->foreground = state->foreground_color;
			cell->background = state->background_color;
		}
	}
	if (point_count != 0)
		result = fb_gfx3_surface_points(surface, &clip, points, point_count);
	free(points);
	if (result == FB_GFX3_OK)
		console_invalidate_shadow(state);
	return result == FB_GFX3_OK;
}

static void console_scroll(fb_ConHooks *hooks, int x1, int y1, int x2,
	int y2, int rows)
{
	FB_GFX3_CONSOLE_PRINT_INFO *info;
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	FB_GFX3_CONSOLE_CELL *cells;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int region_width;
	int region_height;
	int destination_row;
	int clear_y;

	if ((hooks == NULL) || (rows <= 0) || (x1 < 0) || (y1 < 0) ||
	    (x2 < x1) || (y2 < y1))
		return;
	info = (FB_GFX3_CONSOLE_PRINT_INFO *)hooks->Opaque;
	state = (info == NULL) ? NULL : info->state;
	surface = console_page_surface(state);
	cells = console_page_cells(state);
	if ((surface == NULL) || (cells == NULL))
		return;
	region_width = x2 - x1 + 1;
	region_height = y2 - y1 + 1;
	console_full_clip(surface, &clip);
	if (rows < region_height) {
		source_rect.x1 = x1 * (int)console_state.font_width;
		source_rect.y1 = (y1 + rows) * (int)console_state.font_height;
		source_rect.x2 = ((x2 + 1) * (int)console_state.font_width) - 1;
		source_rect.y2 = ((y2 + 1) * (int)console_state.font_height) - 1;
		/*
			Hercules exposes twenty-five 14-pixel text rows in 348 scanlines.
			Its final two character scanlines are intentionally invisible, so
			the scroll source must stop at the physical framebuffer boundary.
		*/
		source_rect.y2 = MIN(source_rect.y2, (int)surface->height - 1);
		fb_gfx3_surface_blit(surface, &clip, surface, &source_rect,
			x1 * (int)console_state.font_width,
			y1 * (int)console_state.font_height, FB_GFX3_BLIT_PSET, 255);
		for (destination_row = y1;
		     destination_row <= y2 - rows; ++destination_row) {
			memmove(cells +
				((size_t)destination_row * console_state.columns) + x1,
				cells + ((size_t)(destination_row + rows) *
				console_state.columns) + x1,
				(size_t)region_width * sizeof(cells[0]));
		}
		clear_y = y2 - rows + 1;
	} else {
		clear_y = y1;
	}
	fb_gfx3_surface_rectangle(surface, &clip,
		x1 * (int)console_state.font_width,
		clear_y * (int)console_state.font_height,
		((x2 + 1) * (int)console_state.font_width) - 1,
		((y2 + 1) * (int)console_state.font_height) - 1,
		state->background_color, 0xFFFFu, TRUE, 0);
	console_clear_cells(state, (uint32_t)x1, (uint32_t)clear_y,
		(uint32_t)x2 + 1u, (uint32_t)y2 + 1u);
	console_invalidate_shadow(state);
	hooks->Coord.Y = hooks->Border.Bottom;
}

/* ------------------------------------------------------------------------- */
/* Runtime hook implementations                                              */
/* ------------------------------------------------------------------------- */

unsigned int fb_GfxColor(unsigned int foreground,
	unsigned int background, int flags)
{
	FB_GFX3_DRAW_STATE *state;
	uint32_t previous = 0;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		if (state->mode->standard_mode == 4) {
			/*
				Olivetti and AT&T mode 4 has one foreground attribute, but
				COLOR may associate that attribute with any of the standard
				sixteen colours. The background remains black.
			*/
			previous = state->mode->standard_foreground_color;
			if ((flags & FB_COLOR_FG_DEFAULT) == 0) {
				uint32_t selected = foreground & 15u;

				state->mode->standard_foreground_color = selected;
				state->mode->palette[1] =
					state->mode->standard_colors[selected];
				(void)fb_gfx3_context_set_palette(&state->mode->context,
					state->mode->palette);
			}
			state->foreground_color = 1u;
			state->background_color = 0u;
		} else {
			if (state->mode->depth <= 8) {
				previous = state->foreground_color |
					(state->background_color << 16);
			} else if (state->mode->depth == 16) {
				previous = fb_gfx3_image_expand_color(2,
					state->foreground_color) | 0xFF000000u;
			} else {
				previous = state->foreground_color;
			}
			if ((flags & FB_COLOR_FG_DEFAULT) == 0)
				state->foreground_color = console_fix_color(state->mode,
					foreground);
			if ((flags & FB_COLOR_BG_DEFAULT) == 0)
				state->background_color = console_fix_color(state->mode,
					background);
		}
	}
	FB_GRAPHICS_UNLOCK();
	return previous;
}

void fb_GfxClear(int mode)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	FB_GFX3_RECT clip;
	int32_t center_x;
	int32_t center_y;
	int full_clear;
	int result;
	int top_row = 0;
	int bottom_row = (int)console_state.rows - 1;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_compat_flush_points(state) != FB_GFX3_OK)) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	surface = console_page_surface(state);
	if (surface == NULL) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	if (mode == (int)0xFFFF0000u) {
		if (state->flags & FB_GFX3_VIEWPORT_SET)
			mode = 1;
		else {
			top_row = fb_ConsoleGetTopRow();
			bottom_row = fb_ConsoleGetBotRow();
			mode = ((top_row == 0) &&
				(bottom_row == (int)console_state.rows - 1)) ? 0 : 2;
		}
	}
	if (mode == 1) {
		clip = state->view;
	} else {
		if (mode == 2) {
			top_row = fb_ConsoleGetTopRow();
			bottom_row = fb_ConsoleGetBotRow();
		} else {
			top_row = 0;
			bottom_row = (int)console_state.rows - 1;
		}
		if (top_row < 0)
			top_row = 0;
		if (bottom_row >= (int)console_state.rows)
			bottom_row = (int)console_state.rows - 1;
		clip.x1 = 0;
		clip.x2 = (int32_t)surface->width - 1;
		clip.y1 = top_row * (int)console_state.font_height;
		clip.y2 = ((bottom_row + 1) * (int)console_state.font_height) - 1;
		clip.y2 = MIN(clip.y2, (int32_t)surface->height - 1);
	}
	full_clear = (clip.x1 == 0) && (clip.y1 == 0) &&
		(clip.x2 == (int32_t)surface->width - 1) &&
		(clip.y2 == (int32_t)surface->height - 1);
	/*
		A partial CLS must preserve dirty shadow pixels outside its clip, so
		upload them first. A full-page CLS overwrites every old pixel and may
		discard those writes; uploading them would only add traffic ahead of the
		clear.
	*/
	if (!full_clear &&
	    (fb_gfx3_compat_commit_shadow(state) != FB_GFX3_OK)) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	result = fb_gfx3_surface_clear(surface, &clip, state->background_color,
		fb_gfx3_compat_primitive_flags(state, surface->depth,
			state->background_color));
	if (result != FB_GFX3_OK) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	if (mode == 1) {
		center_x = clip.x1 + ((clip.x2 - clip.x1 + 1) / 2);
		center_y = clip.y1 + ((clip.y2 - clip.y1 + 1) / 2);
		state->last_x = (float)center_x;
		state->last_y = (float)center_y;
	} else {
		console_clear_cells(state, 0, (uint32_t)top_row,
			console_state.columns, (uint32_t)bottom_row + 1u);
		console_state.cursor_x = 0;
		console_state.cursor_y = (uint32_t)top_row;
		if (mode == 0) {
			center_x = (int32_t)(surface->width / 2u);
			center_y = (int32_t)(surface->height / 2u);
			state->last_x = (float)center_x;
			state->last_y = (float)center_y;
		}
	}
	if (full_clear) {
		/*
			Locked software renderers can now read the known clear value directly
			from RAM. No GPU download is needed for their following POINT/PSET
			loop.
		*/
		if (fb_gfx3_compat_replace_shadow_after_full_clear_graphics_locked(
		    state, state->background_color) != FB_GFX3_OK)
			console_invalidate_shadow(state);
	} else {
		console_invalidate_shadow(state);
	}
	FB_GRAPHICS_UNLOCK();
}

int fb_GfxWidth(int columns, int rows)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	FB_GFX3_RECT clip;
	uint32_t requested_columns;
	uint32_t requested_rows;
	uint32_t font_width;
	uint32_t font_height;
	uint32_t center_x;
	uint32_t center_y;
	int previous;
	int result;

	FB_GRAPHICS_LOCK();
	previous = (int)console_state.columns | ((int)console_state.rows << 16);
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_compat_flush_points(state) != FB_GFX3_OK)) {
		FB_GRAPHICS_UNLOCK();
		return previous;
	}
	if ((state != NULL) && (state->mode != NULL) &&
	    ((columns > 0) || (rows > 0))) {
		requested_columns = (columns > 0) ? (uint32_t)columns :
			console_state.columns;
		requested_rows = (rows > 0) ? (uint32_t)rows : console_state.rows;
		if ((requested_columns != 0u) && (requested_rows != 0u)) {
			font_width = (requested_columns == console_state.columns) ?
				console_state.font_width :
				state->mode->width / requested_columns;
			font_height = (requested_rows == console_state.rows) ?
				console_state.font_height :
				state->mode->height / requested_rows;
			if ((font_width == 8u) || (font_width == 9u)) {
				result = console_set_layout(state->mode, font_width,
					font_height, requested_columns, requested_rows,
					state->foreground_color, state->background_color);
				if (result == FB_GFX3_OK) {
					surface = console_page_surface(state);
					if (surface != NULL) {
						console_full_clip(surface, &clip);
						fb_gfx3_surface_clear(surface, &clip,
							state->background_color,
							fb_gfx3_compat_primitive_flags(state,
							surface->depth, state->background_color));
						console_invalidate_shadow(state);
					}
					state->view.x1 = 0;
					state->view.y1 = 0;
					state->view.x2 = (int32_t)state->mode->width - 1;
					state->view.y2 = (int32_t)state->mode->height - 1;
					state->flags &= ~(uint32_t)(FB_GFX3_WINDOW_ACTIVE |
						FB_GFX3_WINDOW_SCREEN | FB_GFX3_VIEWPORT_SET |
						FB_GFX3_VIEW_SCREEN);
					center_x = state->mode->width / 2u;
					center_y = state->mode->height / 2u;
					state->last_x = (float)center_x;
					state->last_y = (float)center_y;
					fb_ConsoleSetTopBotRows(-1, -1);
				}
			}
		}
	}
	FB_GRAPHICS_UNLOCK();
	return previous;
}

int fb_GfxLocateRaw(int row, int column, int cursor)
{
	int result;

	(void)cursor;
	if ((column >= 0) && (column < (int)console_state.columns))
		console_state.cursor_x = (uint32_t)column;
	if ((row >= 0) && (row < (int)console_state.rows))
		console_state.cursor_y = (uint32_t)row;
	result = (int)console_state.cursor_x |
		((int)console_state.cursor_y << 8);
	return result;
}

int fb_GfxLocate(int row, int column, int cursor)
{
	int result;

	FB_GRAPHICS_LOCK();
	result = fb_GfxLocateRaw(row - 1, column - 1, cursor) + 0x0101;
	fb_SetPos(FB_HANDLE_SCREEN, (int)console_state.cursor_x);
	FB_GRAPHICS_UNLOCK();
	return result;
}

int fb_GfxGetX(void)
{
	return (int)console_state.cursor_x + 1;
}

int fb_GfxGetY(void)
{
	return (int)console_state.cursor_y + 1;
}

void fb_GfxGetXY(int *column, int *row)
{
	if (column != NULL)
		*column = fb_GfxGetX();
	if (row != NULL)
		*row = fb_GfxGetY();
}

void fb_GfxGetSize(int *columns, int *rows)
{
	if (columns != NULL)
		*columns = (int)console_state.columns;
	if (rows != NULL)
		*rows = (int)console_state.rows;
}

void fb_GfxPrintBufferEx(const void *buffer, size_t length, int mask)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_CONSOLE_PRINT_INFO info;
	fb_ConHooks hooks;
	int view_top;
	int view_bottom;

	if ((length == 0) && ((mask & FB_PRINT_FORCE_ADJUST) == 0))
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state == NULL) || !console_state.initialized) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	if (fb_gfx3_compat_flush_points(state) != FB_GFX3_OK) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	fb_ConsoleGetView(&view_top, &view_bottom);
	memset(&hooks, 0, sizeof(hooks));
	info.state = state;
	hooks.Opaque = &info;
	hooks.Scroll = console_scroll;
	hooks.Write = console_write;
	hooks.Border.Left = 0;
	hooks.Border.Top = view_top - 1;
	hooks.Border.Right = (int)console_state.columns - 1;
	hooks.Border.Bottom = view_bottom - 1;
	hooks.Coord.X = (int)console_state.cursor_x;
	hooks.Coord.Y = (int)console_state.cursor_y;
	fb_ConPrintTTY(&hooks, (const char *)buffer, length, TRUE);
	if ((hooks.Coord.X != hooks.Border.Left) ||
	    (hooks.Coord.Y != hooks.Border.Bottom + 1)) {
		fb_hConCheckScroll(&hooks);
	} else {
		hooks.Coord.X = hooks.Border.Right;
		hooks.Coord.Y = hooks.Border.Bottom;
	}
	console_state.cursor_x = (uint32_t)hooks.Coord.X;
	console_state.cursor_y = (uint32_t)hooks.Coord.Y;
	FB_GRAPHICS_UNLOCK();
}

void fb_GfxPrintBuffer(const char *buffer, int mask)
{
	if (buffer != NULL)
		fb_GfxPrintBufferEx(buffer, strlen(buffer), mask);
}

void fb_GfxPrintBufferWstrEx(const FB_WCHAR *buffer, size_t length,
	int mask)
{
	char *temporary;

	if ((buffer == NULL) && (length != 0))
		return;
	if ((length == SIZE_MAX) || (length > (size_t)SSIZE_MAX))
		return;
	temporary = (char *)malloc(length + 1u);
	if (temporary == NULL)
		return;
	if (length != 0)
		fb_wstr_ConvToA(temporary, (ssize_t)length, buffer);
	temporary[length] = '\0';
	fb_GfxPrintBufferEx(temporary, length, mask);
	free(temporary);
}

void fb_GfxPrintBufferWstr(const FB_WCHAR *buffer, int mask)
{
	if (buffer != NULL) {
		ssize_t length = fb_wstr_Len(buffer);

		if (length >= 0)
			fb_GfxPrintBufferWstrEx(buffer, (size_t)length, mask);
	}
}

unsigned int fb_GfxReadXY(int column, int row, int color_flag)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_CONSOLE_CELL *cells;
	FB_GFX3_CONSOLE_CELL *cell;
	uint32_t result = 0;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	cells = console_page_cells(state);
	if ((cells != NULL) && (column > 0) && (row > 0) &&
	    (column <= (int)console_state.columns) &&
	    (row <= (int)console_state.rows)) {
		cell = cells + ((size_t)(row - 1) * console_state.columns) +
			(column - 1);
		if (color_flag == 0) {
			result = cell->character;
		} else if (state->mode->depth <= 4) {
			result = cell->foreground | (cell->background << 4);
		} else if (state->mode->depth <= 8) {
			result = cell->foreground | (cell->background << 8);
		} else if (state->mode->depth == 16) {
			result = fb_gfx3_image_expand_color(2,
				(color_flag == 2) ? cell->background :
				cell->foreground) | 0xFF000000u;
		} else {
			result = (color_flag == 2) ? cell->background :
				cell->foreground;
		}
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

void fb_GfxViewUpdate(void)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_compat_flush_points(state) == FB_GFX3_OK))
		fb_gfx3_surface_present(
			&state->mode->pages[state->mode->visible_page], FALSE);
	FB_GRAPHICS_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* Mode lifecycle and hook ownership                                         */
/* ------------------------------------------------------------------------- */

int fb_gfx3_console_init_locked(FB_GFX3_MODE *mode)
{
	FB_GFX3_DRAW_STATE initial_state;
	int result;

	if ((mode == NULL) || !mode->initialized ||
	    (mode->width < mode->console_font_width) ||
	    (mode->height < mode->console_font_height))
		return FB_GFX3_INVALID;
	memset(&console_state, 0, sizeof(console_state));
	console_state.mode = mode;
	result = console_set_layout(mode, mode->console_font_width,
		mode->console_font_height, 0u, mode->console_rows, 0u, 0u);
	if (result != FB_GFX3_OK) {
		memset(&console_state, 0, sizeof(console_state));
		return result;
	}
	console_state.initialized = TRUE;
	memset(&initial_state, 0, sizeof(initial_state));
	result = fb_gfx3_draw_state_init(mode, &initial_state);
	if (result != FB_GFX3_OK) {
		free(console_state.cells);
		memset(&console_state, 0, sizeof(console_state));
		return result;
	}
	{
		uint32_t page;

		for (page = 0; page < mode->page_count; ++page) {
			initial_state.work_page = page;
			console_clear_cells(&initial_state, 0, 0,
				console_state.columns, console_state.rows);
		}
	}
	__fb_ctx.hooks.clsproc = fb_GfxClear;
	__fb_ctx.hooks.colorproc = fb_GfxColor;
	__fb_ctx.hooks.locateproc = fb_GfxLocate;
	__fb_ctx.hooks.widthproc = fb_GfxWidth;
	__fb_ctx.hooks.getxproc = fb_GfxGetX;
	__fb_ctx.hooks.getyproc = fb_GfxGetY;
	__fb_ctx.hooks.getxyproc = fb_GfxGetXY;
	__fb_ctx.hooks.getsizeproc = fb_GfxGetSize;
	__fb_ctx.hooks.printbuffproc = fb_GfxPrintBufferEx;
	__fb_ctx.hooks.printbuffwproc = fb_GfxPrintBufferWstrEx;
	__fb_ctx.hooks.viewupdateproc = fb_GfxViewUpdate;
	__fb_ctx.hooks.readxyproc = fb_GfxReadXY;
	__fb_ctx.hooks.pagecopyproc = fb_GfxPageCopy;
	__fb_ctx.hooks.pagesetproc = fb_GfxPageSet;
	fb_ConsoleSetTopBotRows(-1, -1);
	return FB_GFX3_OK;
}

void fb_gfx3_console_shutdown_locked(FB_GFX3_MODE *mode)
{
	if (!console_state.initialized || (console_state.mode != mode))
		return;
	memset(&__fb_ctx.hooks, 0, sizeof(__fb_ctx.hooks));
	free(console_state.cells);
	memset(&console_state, 0, sizeof(console_state));
	fb_ConsoleSetTopBotRows(-1, -1);
}

/* end of gfx3_console.c */
