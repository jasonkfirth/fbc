/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_null.c

    Purpose:

        Provide the headless reference backend used to verify common command
        ordering and exact CPU raster behavior before comparing GPU backends.

    Responsibilities:

        - allocate deterministic CPU reference surfaces
        - execute transfers, clear, point, line, box, ellipse, blit, and readback
        - manage surfaces through the common generation-tagged registry
        - provide immediate fence completion for headless tests

    This file intentionally does NOT contain:

        - GPU emulation
        - window or input handling
        - FreeBASIC coordinate and VIEW/WINDOW translation
*/

#include "gfx3_backend_null.h"
#include "gfx3_protocol.h"
#include "gfx3_resource.h"

#include <math.h>

typedef struct FB_GFX3_NULL_SURFACE {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t bytes_per_pixel;
	size_t pitch;
	unsigned char *pixels;
} FB_GFX3_NULL_SURFACE;

typedef struct FB_GFX3_NULL_STATE {
	FB_GFX3_RESOURCE_REGISTRY *resources;
	uint64_t submitted_sequence;
	uint64_t completed_sequence;
} FB_GFX3_NULL_STATE;

/* ------------------------------------------------------------------------- */
/* Surface storage and pixel access                                          */
/* ------------------------------------------------------------------------- */

static uint32_t null_color_mask(uint32_t depth)
{
	if (depth >= 32)
		return UINT32_MAX;
	return (1u << depth) - 1u;
}

static uint32_t null_bytes_per_pixel(uint32_t depth)
{
	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
		return 1;
	case 16:
		return 2;
	case 32:
		return 4;
	default:
		return 0;
	}
}

static void null_surface_destroy(void *resource)
{
	FB_GFX3_NULL_SURFACE *surface = (FB_GFX3_NULL_SURFACE *)resource;

	if (surface == NULL)
		return;
	free(surface->pixels);
	free(surface);
}

static void null_put_pixel(FB_GFX3_NULL_SURFACE *surface, int x, int y,
	uint32_t color)
{
	unsigned char *destination;

	destination = surface->pixels + ((size_t)y * surface->pitch) +
		((size_t)x * surface->bytes_per_pixel);
	color &= null_color_mask(surface->depth);
	switch (surface->bytes_per_pixel) {
	case 1:
		destination[0] = (unsigned char)color;
		break;
	case 2:
		((uint16_t *)destination)[0] = (uint16_t)color;
		break;
	default:
		((uint32_t *)destination)[0] = color;
		break;
	}
}

static uint32_t null_get_pixel(const FB_GFX3_NULL_SURFACE *surface,
	int x, int y)
{
	const unsigned char *source;

	source = surface->pixels + ((size_t)y * surface->pitch) +
		((size_t)x * surface->bytes_per_pixel);
	switch (surface->bytes_per_pixel) {
	case 1:
		return source[0];
	case 2:
		return ((const uint16_t *)source)[0];
	default:
		return ((const uint32_t *)source)[0];
	}
}

static void null_put_primitive_pixel(FB_GFX3_NULL_SURFACE *surface,
	int x, int y, uint32_t color, uint32_t flags)
{
	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) &&
	    (surface->depth == 32))
		color = fb_gfx3_alpha_primitive_pixel(color,
			null_get_pixel(surface, x, y));
	null_put_pixel(surface, x, y, color);
}

static int null_clip_rect(const FB_GFX3_NULL_SURFACE *surface,
	const FB_GFX3_RECT *requested, FB_GFX3_RECT *clipped)
{
	if ((surface == NULL) || (requested == NULL) || (clipped == NULL))
		return FALSE;

	clipped->x1 = requested->x1;
	clipped->y1 = requested->y1;
	clipped->x2 = requested->x2;
	clipped->y2 = requested->y2;
	if (clipped->x1 < 0)
		clipped->x1 = 0;
	if (clipped->y1 < 0)
		clipped->y1 = 0;
	if (clipped->x2 >= (int32_t)surface->width)
		clipped->x2 = (int32_t)surface->width - 1;
	if (clipped->y2 >= (int32_t)surface->height)
		clipped->y2 = (int32_t)surface->height - 1;
	return (clipped->x1 <= clipped->x2) && (clipped->y1 <= clipped->y2);
}

static int null_surface_create(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_HANDLE handle;
	size_t pixel_bytes;
	size_t pitch;
	uint32_t bytes_per_pixel;
	uint32_t y;
	uint32_t x;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->width > 16384) || (payload->height > 16384))
		return FB_GFX3_INVALID;

	bytes_per_pixel = null_bytes_per_pixel(payload->depth);
	if (bytes_per_pixel == 0)
		return FB_GFX3_INVALID;

	if ((fb_gfx3_size_multiply(payload->width, bytes_per_pixel, &pitch) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(pitch, payload->height, &pixel_bytes) !=
	     FB_GFX3_OK) || (pitch == 0u) || (pixel_bytes == 0u))
		return FB_GFX3_INVALID;

	surface = (FB_GFX3_NULL_SURFACE *)calloc(1, sizeof(*surface));
	if (surface == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	surface->pixels = (unsigned char *)malloc(pixel_bytes);
	if (surface->pixels == NULL) {
		free(surface);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	surface->width = payload->width;
	surface->height = payload->height;
	surface->depth = payload->depth;
	surface->bytes_per_pixel = bytes_per_pixel;
	surface->pitch = pitch;

	for (y = 0; y < surface->height; y++) {
		for (x = 0; x < surface->width; x++)
			null_put_pixel(surface, (int)x, (int)y, payload->clear_color);
	}

	handle = fb_gfx3_resource_register(state->resources,
		FB_GFX3_RESOURCE_SURFACE, surface, null_surface_destroy);
	if (handle == 0) {
		null_surface_destroy(surface);
		return FB_GFX3_OUT_OF_MEMORY;
	}

	if (fb_gfx3_completion_set_value(command->completion, 0, handle) !=
	    FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		fb_gfx3_resources_collect(state->resources, UINT64_MAX);
		return FB_GFX3_INVALID;
	}
	return FB_GFX3_OK;
}

static int null_surface_retain_handle(FB_GFX3_NULL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence,
	FB_GFX3_NULL_SURFACE **surface)
{
	int result;

	result = fb_gfx3_resource_retain(state->resources, handle,
		FB_GFX3_RESOURCE_SURFACE, (void **)surface);
	if (result != FB_GFX3_OK)
		return result;

	result = fb_gfx3_resource_mark_used(state->resources, handle, sequence);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		*surface = NULL;
	}
	return result;
}

static int null_surface_retain(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command, FB_GFX3_NULL_SURFACE **surface)
{
	return null_surface_retain_handle(state, command->target,
		command->sequence, surface);
}

static int null_page_set(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->width != surface->width) ||
	    (payload->height != surface->height) ||
	    (payload->depth != surface->depth))
		result = FB_GFX3_INVALID;
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int null_present(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	FB_GFX3_NULL_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	result = null_surface_retain(state, command, &surface);
	if (result == FB_GFX3_OK)
		fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int null_surface_upload(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	size_t header_size = offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data);
	size_t expected_data_size;
	size_t row_size;
	uint32_t y;
	int result;

	if (fb_gfx3_command_payload_size(command) < header_size)
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->destination_x < 0) || (payload->destination_y < 0) ||
	    (payload->data_size !=
		fb_gfx3_command_payload_size(command) - header_size))
		return FB_GFX3_INVALID;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (((uint64_t)(uint32_t)payload->destination_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->destination_y + payload->height >
	     surface->height) ||
	    (fb_gfx3_size_multiply(payload->width, surface->bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) ||
	    (payload->source_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->source_pitch, payload->height,
	     &expected_data_size) != FB_GFX3_OK) ||
	    (expected_data_size != payload->data_size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	for (y = 0; y < payload->height; y++) {
		memcpy(surface->pixels +
			((size_t)(payload->destination_y + (int32_t)y) *
				surface->pitch) +
			((size_t)payload->destination_x * surface->bytes_per_pixel),
			payload->data + ((size_t)y * payload->source_pitch), row_size);
	}
	result = FB_GFX3_OK;

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int null_surface_download(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	unsigned char *destination;
	size_t expected_size;
	size_t row_size;
	uint32_t y;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->source_x < 0) || (payload->source_y < 0) ||
	    (payload->destination_address == 0) ||
	    (payload->destination_address > UINTPTR_MAX))
		return FB_GFX3_INVALID;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (((uint64_t)(uint32_t)payload->source_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->source_y + payload->height >
	     surface->height) ||
	    (fb_gfx3_size_multiply(payload->width, surface->bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) ||
	    (payload->destination_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->destination_pitch, payload->height,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != payload->destination_size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	destination = (unsigned char *)(uintptr_t)payload->destination_address;
	for (y = 0; y < payload->height; y++) {
		memcpy(destination + ((size_t)y * payload->destination_pitch),
			surface->pixels +
				((size_t)(payload->source_y + (int32_t)y) *
					surface->pitch) +
				((size_t)payload->source_x * surface->bytes_per_pixel),
			row_size);
	}
	result = FB_GFX3_OK;

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Reference primitives                                                      */
/* ------------------------------------------------------------------------- */

static int null_clear(FB_GFX3_NULL_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;
	int x;
	int y;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_CLEAR_COMMAND *)command->payload;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;

	if (null_clip_rect(surface, &payload->clip, &clip)) {
		for (y = clip.y1; y <= clip.y2; y++) {
			for (x = clip.x1; x <= clip.x2; x++)
				null_put_primitive_pixel(surface, x, y, payload->color,
					payload->flags);
		}
	}

	fb_gfx3_resource_release(state->resources, command->target);
	return FB_GFX3_OK;
}

static int null_points(FB_GFX3_NULL_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t points_size;
	size_t expected_size;
	uint32_t i;
	int result;

	if (fb_gfx3_command_payload_size(command) <
	    offsetof(FB_GFX3_POINTS_COMMAND, point))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_POINTS_COMMAND *)command->payload;
	if ((fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point), points_size,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;

	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (null_clip_rect(surface, &payload->clip, &clip)) {
		for (i = 0; i < payload->count; i++) {
			if ((payload->point[i].x < clip.x1) ||
			    (payload->point[i].x > clip.x2) ||
			    (payload->point[i].y < clip.y1) ||
			    (payload->point[i].y > clip.y2))
				continue;
			null_put_primitive_pixel(surface, payload->point[i].x,
				payload->point[i].y, payload->point[i].color,
				payload->point[i].flags);
		}
	}

	fb_gfx3_resource_release(state->resources, command->target);
	return FB_GFX3_OK;
}

static uint32_t null_reverse_style(uint32_t style)
{
	style = ((style >> 1) & 0x5555) | ((style & 0x5555) << 1);
	style = ((style >> 2) & 0x3333) | ((style & 0x3333) << 2);
	style = ((style >> 4) & 0x0F0F) | ((style & 0x0F0F) << 4);
	style = ((style >> 8) & 0x00FF) | ((style & 0x00FF) << 8);
	return style;
}

static unsigned int null_rotate_style_bit(unsigned int bit)
{
	return (bit >> 1) | ((bit & 1u) << 15);
}

static unsigned int null_rotate_style(unsigned int bit, int64_t count)
{
	unsigned int rotations = (unsigned int)count & 0xF;

	while (rotations-- != 0)
		bit = null_rotate_style_bit(bit);
	return bit;
}

/* This follows gfxlib2's clipping and style rotation rules. */
static void null_draw_line(FB_GFX3_NULL_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, unsigned int style, uint32_t flags)
{
	int64_t x, y, d, dx, dy, skip, rot;
	int ax, ay;
	int xmin = clip->x1, xmax = clip->x2;
	int ymin = clip->y1, ymax = clip->y2;
	unsigned int bit;
	int64_t dx64 = (int64_t)x2 - x1;
	int64_t dy64 = (int64_t)y2 - y1;

	/*
		The legacy rasterizer performs these calculations with int values.
		Keeping the working values at 64 bits removes those intermediate
		overflows. The remaining bound also guarantees that the products used
		by its fast clipping path fit in a signed 64-bit value.
	*/
	if ((dx64 < -(INT_MAX / 2)) || (dx64 > (INT_MAX / 2)) ||
	    (dy64 < -(INT_MAX / 2)) || (dy64 > (INT_MAX / 2)))
		return;
	if (((x1 < xmin) && (x2 < xmin)) || ((x1 > xmax) && (x2 > xmax)) ||
	    ((y1 < ymin) && (y2 < ymin)) || ((y1 > ymax) && (y2 > ymax)))
		return;

	dx = dx64;
	dy = dy64;
	if (x2 < xmin)
		x2 = xmin;
	else if (x2 > xmax)
		x2 = xmax;
	if (y2 < ymin)
		y2 = ymin;
	else if (y2 > ymax)
		y2 = ymax;
	rot = 0;

	if (dx == 0) {
		if (y1 < ymin) {
			rot += ymin - y1;
			y1 = ymin;
		} else if (y1 > ymax) {
			rot += y1 - ymax;
			y1 = ymax;
		}
		if (y1 > y2) {
			int swap = y1;
			style = null_reverse_style(style);
			rot = (~rot) + y2 - y1;
			y1 = y2;
			y2 = swap;
		}
		bit = 0x8000u >> ((unsigned int)rot & 0xF);
		for (y = y1; y <= y2; y++) {
			if (style & bit)
				null_put_primitive_pixel(surface, x1, (int)y, color,
					flags);
			bit = null_rotate_style_bit(bit);
		}
	} else if (dy == 0) {
		if (x1 < xmin) {
			rot += xmin - x1;
			x1 = xmin;
		} else if (x1 > xmax) {
			rot += x1 - xmax;
			x1 = xmax;
		}
		if (x1 > x2) {
			int swap = x1;
			style = null_reverse_style(style);
			rot = (~rot) + x2 - x1;
			x1 = x2;
			x2 = swap;
		}
		bit = 0x8000u >> ((unsigned int)rot & 0xF);
		for (x = x1; x <= x2; x++) {
			if (style & bit)
				null_put_primitive_pixel(surface, (int)x, y1, color,
					flags);
			bit = null_rotate_style_bit(bit);
		}
	} else {
		ax = ay = 1;
		if (dx < 0) {
			dx = -dx;
			ax = -1;
		}
		if (dy < 0) {
			dy = -dy;
			ay = -1;
		}

		d = (dx >= dy) ? dy * 2 - dx : dy - dx * 2;
		dx *= 2;
		dy *= 2;
		x = x1 < xmin ? xmin : (x1 > xmax ? xmax : x1);
		d += ax * (x - x1) * dy;
		y = y1 < ymin ? ymin : (y1 > ymax ? ymax : y1);
		d -= ay * (y - y1) * dx;
		if (dx >= dy)
			rot += ax * (x - x1);
		else
			rot += ay * (y - y1);
		x2 += ax;
		y2 += ay;

		if (dx >= dy) {
			if (d >= dy) {
				skip = (d - dy) / dx + 1;
				y += ay * skip;
				d -= skip * dx;
				if ((y < ymin) || (y > ymax))
					return;
			} else if (d < (dy - dx)) {
				skip = ((dy - dx) - d) / dy + 1;
				x += ax * skip;
				d += skip * dy;
				rot += skip;
				if ((x < xmin) || (x > xmax))
					return;
			}
			bit = 0x8000u >> ((unsigned int)rot & 0xF);
			while ((x != x2) && (y != y2)) {
				if (style & bit)
					null_put_primitive_pixel(surface, (int)x, (int)y,
						color, flags);
				bit = null_rotate_style_bit(bit);
				if (d >= 0) {
					y += ay;
					d -= dx;
				}
				d += dy;
				x += ax;
			}
		} else {
			if (d < -dx) {
				skip = (-dx - d) / dy + 1;
				x += ax * skip;
				d += skip * dy;
				if ((x < xmin) || (x > xmax))
					return;
			} else if (d > dy - dx) {
				skip = (d - (dy - dx)) / dx + 1;
				y += ay * skip;
				d -= skip * dx;
				rot += skip;
				if ((y < ymin) || (y > ymax))
					return;
			}
			bit = 0x8000u >> ((unsigned int)rot & 0xF);
			while ((y != y2) && (x != x2)) {
				if (style & bit)
					null_put_primitive_pixel(surface, (int)x, (int)y,
						color, flags);
				bit = null_rotate_style_bit(bit);
				if (d <= 0) {
					x += ax;
					d += dy;
				}
				d -= dx;
				y += ay;
			}
		}
	}
}

static int null_line(FB_GFX3_NULL_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINE_COMMAND *)command->payload;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (null_clip_rect(surface, &payload->clip, &clip))
		null_draw_line(surface, &clip, payload->x1, payload->y1,
			payload->x2, payload->y2, payload->color, payload->style,
			payload->flags);
	fb_gfx3_resource_release(state->resources, command->target);
	return FB_GFX3_OK;
}

/*
	Box styles continue around the complete perimeter, including portions that
	are clipped away. This is observably different from drawing four unrelated
	LINE commands and follows gfxlib2's fb_hGfxBox() behavior.
*/
static void null_draw_box(FB_GFX3_NULL_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_RECTANGLE_COMMAND *payload)
{
	FB_GFX3_RECT box;
	FB_GFX3_RECT clipped;
	unsigned int bit = 0x8000u;
	uint32_t style = payload->style & 0xFFFFu;
	int x;
	int y;

	box.x1 = payload->x1;
	box.y1 = payload->y1;
	box.x2 = payload->x2;
	box.y2 = payload->y2;
	if ((box.x2 < clip->x1) || (box.y2 < clip->y1) ||
	    (box.x1 > clip->x2) || (box.y1 > clip->y2))
		return;

	clipped.x1 = (box.x1 < clip->x1) ? clip->x1 : box.x1;
	clipped.y1 = (box.y1 < clip->y1) ? clip->y1 : box.y1;
	clipped.x2 = (box.x2 > clip->x2) ? clip->x2 : box.x2;
	clipped.y2 = (box.y2 > clip->y2) ? clip->y2 : box.y2;

	if (payload->filled != 0) {
		for (y = clipped.y1; y <= clipped.y2; y++) {
			for (x = clipped.x1; x <= clipped.x2; x++)
				null_put_primitive_pixel(surface, x, y, payload->color,
					payload->flags);
		}
		return;
	}

	bit = null_rotate_style(bit, (int64_t)clipped.x1 - box.x1);
	if (box.y2 <= clip->y2) {
		for (x = clipped.x1; x <= clipped.x2; x++) {
			if (style & bit)
				null_put_primitive_pixel(surface, x, box.y2,
					payload->color, payload->flags);
			bit = null_rotate_style_bit(bit);
		}
	} else {
		bit = null_rotate_style(bit,
			(int64_t)clipped.x2 - clipped.x1 + 1);
	}
	bit = null_rotate_style(bit,
		((int64_t)box.x2 - clipped.x2) + (clipped.x1 - box.x1));

	if (box.y1 >= clip->y1) {
		for (x = clipped.x1; x <= clipped.x2; x++) {
			if (style & bit)
				null_put_primitive_pixel(surface, x, box.y1,
					payload->color, payload->flags);
			bit = null_rotate_style_bit(bit);
		}
	} else {
		bit = null_rotate_style(bit,
			(int64_t)clipped.x2 - clipped.x1 + 1);
	}
	bit = null_rotate_style(bit,
		((int64_t)box.x2 - clipped.x2) + (clipped.y1 - box.y1));

	if (box.x2 <= clip->x2) {
		for (y = clipped.y1; y <= clipped.y2; y++) {
			if (style & bit)
				null_put_primitive_pixel(surface, box.x2, y,
					payload->color, payload->flags);
			bit = null_rotate_style_bit(bit);
		}
	} else {
		bit = null_rotate_style(bit,
			(int64_t)clipped.y2 - clipped.y1 + 1);
	}
	bit = null_rotate_style(bit,
		((int64_t)box.y2 - clipped.y2) + (clipped.y1 - box.y1));

	if (box.x1 >= clip->x1) {
		for (y = clipped.y1; y <= clipped.y2; y++) {
			if (style & bit)
				null_put_primitive_pixel(surface, box.x1, y,
					payload->color, payload->flags);
			bit = null_rotate_style_bit(bit);
		}
	}
}

static int null_rectangle(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	if ((payload->x1 > payload->x2) || (payload->y1 > payload->y2))
		return FB_GFX3_INVALID;

	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (null_clip_rect(surface, &payload->clip, &clip))
		null_draw_box(surface, &clip, payload);
	fb_gfx3_resource_release(state->resources, command->target);
	return FB_GFX3_OK;
}

static void null_draw_ellipse_scanline(FB_GFX3_NULL_SURFACE *surface,
	const FB_GFX3_RECT *clip, int y, int x1, int x2, uint32_t color,
	int filled, uint32_t flags)
{
	int x;

	if ((y < clip->y1) || (y > clip->y2))
		return;
	if (filled) {
		if ((x2 < clip->x1) || (x1 > clip->x2))
			return;
		if (x1 < clip->x1)
			x1 = clip->x1;
		if (x2 > clip->x2)
			x2 = clip->x2;
		for (x = x1; x <= x2; x++)
			null_put_primitive_pixel(surface, x, y, color, flags);
		return;
	}

	if ((x1 >= clip->x1) && (x1 <= clip->x2))
		null_put_primitive_pixel(surface, x1, y, color, flags);
	if ((x2 >= clip->x1) && (x2 <= clip->x2))
		null_put_primitive_pixel(surface, x2, y, color, flags);
}

/* This is gfxlib2's midpoint ellipse rasterizer with wider intermediates. */
static int null_ellipse(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int d;
	int x1;
	int x2;
	int y1;
	int y2;
	int64_t aq;
	int64_t bq;
	int64_t dx;
	int64_t dy;
	int64_t r;
	int64_t rx;
	int64_t ry;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	if (!(payload->radius_x >= 0.0f) ||
	    !(payload->radius_x <= 32767.0f) ||
	    !(payload->radius_y >= 0.0f) ||
	    !(payload->radius_y <= 32767.0f))
		return FB_GFX3_INVALID;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!null_clip_rect(surface, &payload->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	x1 = (int)((float)payload->center_x - payload->radius_x);
	x2 = (int)((float)payload->center_x + payload->radius_x);
	y1 = payload->center_y;
	y2 = payload->center_y;
	if (payload->radius_y == 0.0f) {
		null_draw_ellipse_scanline(surface, &clip, y1, x1, x2,
			payload->color, TRUE, payload->flags);
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	null_draw_ellipse_scanline(surface, &clip, y1, x1, x2,
		payload->color, payload->filled != 0, payload->flags);
	aq = (int64_t)(payload->radius_x * payload->radius_x);
	bq = (int64_t)(payload->radius_y * payload->radius_y);
	dx = aq * 2;
	dy = bq * 2;
	r = (int64_t)(payload->radius_x * (float)bq);
	rx = r * 2;
	ry = 0;
	d = (int)payload->radius_x;

	while (d > 0) {
		if (r > 0) {
			y1++;
			y2--;
			ry += dx;
			r -= ry;
		}
		if (r <= 0) {
			d--;
			x1++;
			x2--;
			rx -= dy;
			r += rx;
		}
		null_draw_ellipse_scanline(surface, &clip, y1, x1, x2,
			payload->color, payload->filled != 0, payload->flags);
		null_draw_ellipse_scanline(surface, &clip, y2, x1, x2,
			payload->color, payload->filled != 0, payload->flags);
	}

	fb_gfx3_resource_release(state->resources, command->target);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Reference surface blits                                                   */
/* ------------------------------------------------------------------------- */

static int null_blit_pixel(uint32_t depth, uint32_t mode, uint32_t source,
	uint32_t destination, uint32_t blend_alpha, uint32_t *output,
	int *write_pixel)
{
	uint32_t alpha;
	uint32_t destination_rb;
	uint32_t destination_ga;
	uint32_t source_rb;
	uint32_t source_ga;
	uint32_t temporary1;
	uint32_t temporary2;
	uint32_t overflow;
	uint32_t mask = null_color_mask(depth);

	*write_pixel = TRUE;
	switch (mode) {
	case FB_GFX3_BLIT_PSET:
		*output = source & mask;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_PRESET:
		*output = (~source) & mask;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_AND:
		*output = (source & destination) & mask;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_OR:
		*output = (source | destination) & mask;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_XOR:
		*output = (source ^ destination) & mask;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_TRANS:
		if (depth <= 8) {
			if ((source & mask) == 0)
				*write_pixel = FALSE;
			*output = source & mask;
		} else if (depth == 16) {
			if ((source & 0xFFFFu) == 0xF81Fu)
				*write_pixel = FALSE;
			*output = source & 0xFFFFu;
		} else {
			*output = source & 0x00FFFFFFu;
			if (*output == 0x00FF00FFu)
				*write_pixel = FALSE;
		}
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_ALPHA:
		if (depth != 32) {
			*output = source & mask;
			return FB_GFX3_OK;
		}
		/* This is the integer blend performed by gfxlib2's ALPHA putter. */
		alpha = (source >> 24) + 1;
		source_rb = source & 0x00FF00FFu;
		source_ga = source & 0xFF00FF00u;
		destination_rb = destination & 0x00FF00FFu;
		destination_ga = destination & 0xFF00FF00u;
		source_rb = ((source_rb - destination_rb) * alpha) >> 8;
		source_ga = ((source_ga >> 8) - (destination_ga >> 8)) * alpha;
		*output = ((destination_rb + source_rb) & 0x00FF00FFu) |
			((destination_ga + source_ga) & 0xFF00FF00u);
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_ADD:
		alpha = blend_alpha & 0xFFu;
		if (depth <= 8) {
			*output = (source | destination) & mask;
			return FB_GFX3_OK;
		}
		if (depth == 16) {
			if ((source & 0xFFFFu) == 0xF81Fu) {
				*write_pixel = FALSE;
				return FB_GFX3_OK;
			}
			alpha = (alpha + 7) >> 3;
			source = ((source << 16) | source) & 0x07C0F81Fu;
			source = ((source * alpha) >> 5) & 0x07C0F81Fu;
			destination = ((destination << 16) | destination) &
				0x07C0F81Fu;
			source += destination;
			overflow = source & 0x08010020u;
			overflow -= overflow >> 5;
			source |= overflow;
			source &= 0x07C0F81Fu;
			source |= source >> 16;
			*output = source & 0xFFFFu;
			return FB_GFX3_OK;
		}
		if ((source & 0x00FFFFFFu) == 0x00FF00FFu) {
			*write_pixel = FALSE;
			return FB_GFX3_OK;
		}
		temporary1 = source & 0x00FF00FFu;
		temporary2 = (source >> 8) & 0x00FF00FFu;
		temporary1 = ((temporary1 * alpha) >> 8) & 0x00FF00FFu;
		temporary2 = (temporary2 * alpha) & 0xFF00FF00u;
		source = temporary1 | temporary2;
		temporary1 = source & 0x80808080u;
		temporary2 = destination & 0x80808080u;
		source = (source & 0x7F7F7F7Fu) +
			(destination & 0x7F7F7F7Fu);
		destination = temporary1;
		temporary1 |= temporary2;
		temporary2 = destination & temporary2;
		destination = temporary1 & source;
		source |= ((((temporary2 | destination) >> 7) + 0x7F7F7F7Fu) ^
			0x7F7F7F7Fu) | temporary1;
		*output = source;
		return FB_GFX3_OK;
	case FB_GFX3_BLIT_BLEND:
		alpha = blend_alpha & 0xFFu;
		if (alpha == 0) {
			*write_pixel = FALSE;
			return FB_GFX3_OK;
		}
		if (depth <= 8) {
			if ((source & mask) == 0)
				*write_pixel = FALSE;
			*output = source & mask;
			return FB_GFX3_OK;
		}
		if (depth == 16) {
			if ((source & 0xFFFFu) == 0xF81Fu) {
				*write_pixel = FALSE;
				return FB_GFX3_OK;
			}
			alpha = (alpha + 7) >> 3;
			source_rb = source & 0xF81Fu;
			source_ga = source & 0x07E0u;
			destination_rb = destination & 0xF81Fu;
			destination_ga = destination & 0x07E0u;
			source_rb = ((source_rb - destination_rb) * alpha) >> 5;
			source_ga = ((source_ga - destination_ga) * alpha) >> 5;
			*output = ((destination_rb + source_rb) & 0xF81Fu) |
				((destination_ga + source_ga) & 0x07E0u);
			return FB_GFX3_OK;
		}
		if ((source & 0x00FFFFFFu) == 0x00FF00FFu) {
			*write_pixel = FALSE;
			return FB_GFX3_OK;
		}
		alpha++;
		source_rb = source & 0x00FF00FFu;
		source_ga = source & 0xFF00FF00u;
		destination_rb = destination & 0x00FF00FFu;
		destination_ga = destination & 0xFF00FF00u;
		source_rb = ((source_rb - destination_rb) * alpha) >> 8;
		source_ga = ((source_ga >> 8) - (destination_ga >> 8)) * alpha;
		*output = ((destination_rb + source_rb) & 0x00FF00FFu) |
			((destination_ga + source_ga) & 0xFF00FF00u);
		return FB_GFX3_OK;
	default:
		return FB_GFX3_UNSUPPORTED;
	}
}

static int null_blit(FB_GFX3_NULL_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *destination;
	FB_GFX3_NULL_SURFACE *source;
	FB_GFX3_RECT clip;
	uint32_t *snapshot = NULL;
	size_t snapshot_bytes;
	size_t snapshot_count;
	size_t snapshot_height;
	size_t snapshot_width;
	int64_t destination_x2;
	int64_t destination_y2;
	int start_x;
	int start_y;
	int end_x;
	int end_y;
	int x;
	int y;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	switch (payload->mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
	case FB_GFX3_BLIT_ALPHA:
	case FB_GFX3_BLIT_ADD:
	case FB_GFX3_BLIT_BLEND:
		break;
	default:
		return FB_GFX3_UNSUPPORTED;
	}

	result = null_surface_retain(state, command, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = null_surface_retain_handle(state, payload->source,
		command->sequence, &source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}

	if ((source->depth != destination->depth) ||
	    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
	    (payload->source_rect.x1 > payload->source_rect.x2) ||
	    (payload->source_rect.y1 > payload->source_rect.y2) ||
	    (payload->source_rect.x2 >= (int32_t)source->width) ||
	    (payload->source_rect.y2 >= (int32_t)source->height)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (!null_clip_rect(destination, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}

	destination_x2 = (int64_t)payload->destination_x +
		(payload->source_rect.x2 - payload->source_rect.x1);
	destination_y2 = (int64_t)payload->destination_y +
		(payload->source_rect.y2 - payload->source_rect.y1);
	start_x = (payload->destination_x < clip.x1) ?
		clip.x1 : payload->destination_x;
	start_y = (payload->destination_y < clip.y1) ?
		clip.y1 : payload->destination_y;
	end_x = (destination_x2 > clip.x2) ? clip.x2 : (int)destination_x2;
	end_y = (destination_y2 > clip.y2) ? clip.y2 : (int)destination_y2;
	if ((start_x > end_x) || (start_y > end_y)) {
		result = FB_GFX3_OK;
		goto done;
	}

	if (payload->source == command->target) {
		snapshot_width = (size_t)((int64_t)end_x - start_x + 1);
		snapshot_height = (size_t)((int64_t)end_y - start_y + 1);
		if ((fb_gfx3_size_multiply(snapshot_width, snapshot_height,
		     &snapshot_count) !=
		     FB_GFX3_OK) ||
		    (snapshot_count == 0u) ||
		    (fb_gfx3_size_multiply(snapshot_count, sizeof(*snapshot),
		     &snapshot_bytes) != FB_GFX3_OK) ||
		    (snapshot_bytes == 0u)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		snapshot = (uint32_t *)malloc(snapshot_bytes);
		if (snapshot == NULL) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		for (y = start_y; y <= end_y; y++) {
			for (x = start_x; x <= end_x; x++) {
				snapshot[(size_t)(y - start_y) *
					snapshot_width +
					(size_t)(x - start_x)] =
					null_get_pixel(source,
					payload->source_rect.x1 +
						x - payload->destination_x,
					payload->source_rect.y1 +
						y - payload->destination_y);
			}
		}
	}

	for (y = start_y; y <= end_y; y++) {
		for (x = start_x; x <= end_x; x++) {
			uint32_t source_color;
			uint32_t destination_color;
			uint32_t output;
			int write_pixel;

			if (snapshot != NULL) {
				source_color = snapshot[(size_t)(y - start_y) *
					(size_t)(end_x - start_x + 1) +
					(size_t)(x - start_x)];
			} else {
				source_color = null_get_pixel(source,
					payload->source_rect.x1 +
						x - payload->destination_x,
					payload->source_rect.y1 +
						y - payload->destination_y);
			}
			destination_color = null_get_pixel(destination, x, y);
			result = null_blit_pixel(destination->depth, payload->mode,
				source_color, destination_color, payload->alpha, &output,
				&write_pixel);
			if (result != FB_GFX3_OK)
				goto done;
			if (write_pixel)
				null_put_pixel(destination, x, y, output);
		}
	}
	result = FB_GFX3_OK;

done:
	free(snapshot);
	fb_gfx3_resource_release(state->resources, payload->source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int null_transform_wrap_coordinate(int value, int origin, int size)
{
	int relative = (value - origin) % size;

	if (relative < 0)
		relative += size;
	return origin + relative;
}

static uint32_t null_transform_source_pixel(
	const FB_GFX3_NULL_SURFACE *source, const uint32_t *snapshot,
	const FB_GFX3_RECT *source_rect, int x, int y, uint32_t wrap)
{
	if (wrap == FB_GFX3_TRANSFORM_WRAP_REPEAT) {
		x = null_transform_wrap_coordinate(x, source_rect->x1,
			source_rect->x2 - source_rect->x1 + 1);
		y = null_transform_wrap_coordinate(y, source_rect->y1,
			source_rect->y2 - source_rect->y1 + 1);
	} else {
		if (x < source_rect->x1)
			x = source_rect->x1;
		if (y < source_rect->y1)
			y = source_rect->y1;
		if (x > source_rect->x2)
			x = source_rect->x2;
		if (y > source_rect->y2)
			y = source_rect->y2;
	}
	if (snapshot != NULL)
		return snapshot[(size_t)y * source->width + (size_t)x];
	return null_get_pixel(source, x, y);
}

static uint32_t null_transform_interpolate(uint32_t depth,
	uint32_t top_left, uint32_t top_right, uint32_t bottom_left,
	uint32_t bottom_right, double fraction_x, double fraction_y)
{
	uint32_t result = 0u;
	uint32_t channel_count;
	uint32_t channel;

	if (depth <= 8u)
		return top_left;
	if (depth == 16u) {
		uint32_t colors[4];
		uint32_t red;
		uint32_t green;
		uint32_t blue;

		colors[0] = (((top_left >> 11) & 31u) * 255u / 31u) << 16 |
			(((top_left >> 5) & 63u) * 255u / 63u) << 8 |
			((top_left & 31u) * 255u / 31u);
		colors[1] = (((top_right >> 11) & 31u) * 255u / 31u) << 16 |
			(((top_right >> 5) & 63u) * 255u / 63u) << 8 |
			((top_right & 31u) * 255u / 31u);
		colors[2] = (((bottom_left >> 11) & 31u) * 255u / 31u) << 16 |
			(((bottom_left >> 5) & 63u) * 255u / 63u) << 8 |
			((bottom_left & 31u) * 255u / 31u);
		colors[3] = (((bottom_right >> 11) & 31u) * 255u / 31u) << 16 |
			(((bottom_right >> 5) & 63u) * 255u / 63u) << 8 |
			((bottom_right & 31u) * 255u / 31u);
		red = (uint32_t)((((1.0 - fraction_y) *
			(((1.0 - fraction_x) * ((colors[0] >> 16) & 255u)) +
			 (fraction_x * ((colors[1] >> 16) & 255u)))) +
			(fraction_y * (((1.0 - fraction_x) *
			 ((colors[2] >> 16) & 255u)) +
			 (fraction_x * ((colors[3] >> 16) & 255u))))) + 0.5);
		green = (uint32_t)((((1.0 - fraction_y) *
			(((1.0 - fraction_x) * ((colors[0] >> 8) & 255u)) +
			 (fraction_x * ((colors[1] >> 8) & 255u)))) +
			(fraction_y * (((1.0 - fraction_x) *
			 ((colors[2] >> 8) & 255u)) +
			 (fraction_x * ((colors[3] >> 8) & 255u))))) + 0.5);
		blue = (uint32_t)((((1.0 - fraction_y) *
			(((1.0 - fraction_x) * (colors[0] & 255u)) +
			 (fraction_x * (colors[1] & 255u)))) +
			(fraction_y * (((1.0 - fraction_x) *
			 (colors[2] & 255u)) +
			 (fraction_x * (colors[3] & 255u))))) + 0.5);
		return ((red * 31u / 255u) << 11) |
			((green * 63u / 255u) << 5) | (blue * 31u / 255u);
	}
	channel_count = 4u;
	for (channel = 0u; channel < channel_count; ++channel) {
		uint32_t shift = channel * 8u;
		double top = ((1.0 - fraction_x) *
			((top_left >> shift) & 255u)) +
			(fraction_x * ((top_right >> shift) & 255u));
		double bottom = ((1.0 - fraction_x) *
			((bottom_left >> shift) & 255u)) +
			(fraction_x * ((bottom_right >> shift) & 255u));
		uint32_t value = (uint32_t)(((1.0 - fraction_y) * top) +
			(fraction_y * bottom) + 0.5);

		result |= (value & 255u) << shift;
	}
	return result;
}

static uint32_t null_transform_sample(const FB_GFX3_NULL_SURFACE *source,
	const uint32_t *snapshot, const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload,
	double source_x, double source_y)
{
	int base_x;
	int base_y;
	double fraction_x;
	double fraction_y;

	if ((payload->filter == FB_GFX3_TRANSFORM_FILTER_NEAREST) ||
	    (source->depth <= 8u))
		return null_transform_source_pixel(source, snapshot,
			&payload->source_rect, (int)floor(source_x),
			(int)floor(source_y), payload->wrap);
	source_x -= 0.5;
	source_y -= 0.5;
	base_x = (int)floor(source_x);
	base_y = (int)floor(source_y);
	fraction_x = source_x - floor(source_x);
	fraction_y = source_y - floor(source_y);
	return null_transform_interpolate(source->depth,
		null_transform_source_pixel(source, snapshot, &payload->source_rect,
			base_x, base_y, payload->wrap),
		null_transform_source_pixel(source, snapshot, &payload->source_rect,
			base_x + 1, base_y, payload->wrap),
		null_transform_source_pixel(source, snapshot, &payload->source_rect,
			base_x, base_y + 1, payload->wrap),
		null_transform_source_pixel(source, snapshot, &payload->source_rect,
			base_x + 1, base_y + 1, payload->wrap),
		fraction_x, fraction_y);
}

static int null_transform_blit(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *destination;
	FB_GFX3_NULL_SURFACE *source;
	FB_GFX3_RECT clip;
	uint32_t *snapshot = NULL;
	size_t snapshot_count;
	size_t snapshot_bytes;
	int start_x;
	int start_y;
	int end_x;
	int end_y;
	int x;
	int y;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;
	result = null_surface_retain(state, command, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = null_surface_retain_handle(state, payload->source,
		command->sequence, &source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}
	if ((source->depth != destination->depth) ||
	    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
	    (payload->source_rect.x1 > payload->source_rect.x2) ||
	    (payload->source_rect.y1 > payload->source_rect.y2) ||
	    (payload->source_rect.x2 >= (int32_t)source->width) ||
	    (payload->source_rect.y2 >= (int32_t)source->height) ||
	    !null_clip_rect(destination, &payload->clip, &clip)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	start_x = (payload->destination_bounds.x1 < clip.x1) ? clip.x1 :
		payload->destination_bounds.x1;
	start_y = (payload->destination_bounds.y1 < clip.y1) ? clip.y1 :
		payload->destination_bounds.y1;
	end_x = (payload->destination_bounds.x2 > clip.x2) ? clip.x2 :
		payload->destination_bounds.x2;
	end_y = (payload->destination_bounds.y2 > clip.y2) ? clip.y2 :
		payload->destination_bounds.y2;
	if ((start_x > end_x) || (start_y > end_y)) {
		result = FB_GFX3_OK;
		goto done;
	}
	if (payload->source == command->target) {
		if ((fb_gfx3_size_multiply(source->width, source->height,
		     &snapshot_count) != FB_GFX3_OK) ||
		    (snapshot_count == 0u) ||
		    (fb_gfx3_size_multiply(snapshot_count, sizeof(*snapshot),
		     &snapshot_bytes) != FB_GFX3_OK) ||
		    (snapshot_bytes == 0u)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		snapshot = (uint32_t *)malloc(snapshot_bytes);
		if (snapshot == NULL) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		for (y = 0; y < (int)source->height; ++y) {
			for (x = 0; x < (int)source->width; ++x)
				snapshot[(size_t)y * source->width + (size_t)x] =
					null_get_pixel(source, x, y);
		}
	}
	for (y = start_y; y <= end_y; ++y) {
		for (x = start_x; x <= end_x; ++x) {
			double pixel_x = (double)x + 0.5;
			double pixel_y = (double)y + 0.5;
			double denominator = (payload->inverse[6] * pixel_x) +
				(payload->inverse[7] * pixel_y) + payload->inverse[8];
			double source_x;
			double source_y;
			uint32_t source_color;
			uint32_t destination_color;
			uint32_t output;
			int write_pixel;

			if (denominator <= 0.000001)
				continue;
			source_x = ((payload->inverse[0] * pixel_x) +
				(payload->inverse[1] * pixel_y) + payload->inverse[2]) /
				denominator;
			source_y = ((payload->inverse[3] * pixel_x) +
				(payload->inverse[4] * pixel_y) + payload->inverse[5]) /
				denominator;
			if ((payload->wrap == FB_GFX3_TRANSFORM_WRAP_CLAMP) &&
			    ((source_x < payload->source_rect.x1) ||
			     (source_y < payload->source_rect.y1) ||
			     (source_x >= (double)payload->source_rect.x2 + 1.0) ||
			     (source_y >= (double)payload->source_rect.y2 + 1.0)))
				continue;
			source_color = null_transform_sample(source, snapshot, payload,
				source_x, source_y);
			destination_color = null_get_pixel(destination, x, y);
			result = null_blit_pixel(destination->depth, payload->mode,
				source_color, destination_color, payload->alpha, &output,
				&write_pixel);
			if (result != FB_GFX3_OK)
				goto done;
			if (write_pixel)
				null_put_pixel(destination, x, y, output);
		}
	}
	result = FB_GFX3_OK;

done:
	free(snapshot);
	fb_gfx3_resource_release(state->resources, payload->source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int null_read_pixel(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_NULL_SURFACE *surface;
	uint32_t color = UINT32_MAX;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	result = null_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->x >= 0) && (payload->y >= 0) &&
	    (payload->x < (int32_t)surface->width) &&
	    (payload->y < (int32_t)surface->height))
		color = null_get_pixel(surface, payload->x, payload->y);
	fb_gfx3_resource_release(state->resources, command->target);
	return fb_gfx3_completion_set_value(command->completion, 0, color);
}

/* ------------------------------------------------------------------------- */
/* Backend interface                                                         */
/* ------------------------------------------------------------------------- */

static int null_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	if (caps == NULL)
		return FB_GFX3_INVALID;

	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->features = FB_GFX3_FEATURE_INDEXED_SURFACES;
	caps->max_surface_width = 16384;
	caps->max_surface_height = 16384;
	caps->max_batch_commands = 4096;
	return FB_GFX3_OK;
}

static int null_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	FB_GFX3_NULL_STATE *state;

	if ((backend == NULL) || (config == NULL) || (config->resources == NULL) ||
	    (config->width == 0) || (config->height == 0) ||
	    (config->page_count == 0) ||
	    (config->width > backend->caps.max_surface_width) ||
	    (config->height > backend->caps.max_surface_height))
		return FB_GFX3_INVALID;

	switch (config->depth) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
		break;
	default:
		return FB_GFX3_INVALID;
	}

	state = (FB_GFX3_NULL_STATE *)calloc(1, sizeof(*state));
	if (state == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	state->resources = config->resources;
	backend->state = state;
	return FB_GFX3_OK;
}

static void null_shutdown(FB_GFX3_BACKEND *backend)
{
	if (backend == NULL)
		return;
	free(backend->state);
	backend->state = NULL;
}

static int null_execute_one(FB_GFX3_NULL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	int result;

	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_CREATE:
		return null_surface_create(state, command);
	case FB_GFX3_COMMAND_SURFACE_DESTROY:
		if (fb_gfx3_command_payload_size(command) != 0)
			return FB_GFX3_INVALID;
		result = fb_gfx3_resource_mark_used(state->resources,
			command->target, command->sequence);
		if (result != FB_GFX3_OK)
			return result;
		return fb_gfx3_resource_release(state->resources, command->target);
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
		return null_surface_upload(state, command);
	case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
		return null_surface_download(state, command);
	case FB_GFX3_COMMAND_CLEAR:
		return null_clear(state, command);
	case FB_GFX3_COMMAND_POINTS:
		return null_points(state, command);
	case FB_GFX3_COMMAND_LINE:
		return null_line(state, command);
	case FB_GFX3_COMMAND_RECTANGLE:
		return null_rectangle(state, command);
	case FB_GFX3_COMMAND_ELLIPSE:
		return null_ellipse(state, command);
	case FB_GFX3_COMMAND_BLIT:
		return null_blit(state, command);
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		return null_transform_blit(state, command);
	case FB_GFX3_COMMAND_READ_PIXEL:
		return null_read_pixel(state, command);
	case FB_GFX3_COMMAND_PALETTE:
		return (fb_gfx3_command_payload_size(command) ==
			sizeof(FB_GFX3_PALETTE_COMMAND)) ?
			FB_GFX3_OK : FB_GFX3_INVALID;
	case FB_GFX3_COMMAND_PAGE_SET:
		return null_page_set(state, command);
	case FB_GFX3_COMMAND_PRESENT:
		return null_present(state, command);
	case FB_GFX3_COMMAND_WINDOW_TITLE:
		{
			const FB_GFX3_WINDOW_TITLE_COMMAND *payload;
			size_t payload_size = fb_gfx3_command_payload_size(command);
			size_t expected_size;

			if (payload_size < sizeof(*payload))
				return FB_GFX3_INVALID;
			payload = (const FB_GFX3_WINDOW_TITLE_COMMAND *)
				command->payload;
			if ((fb_gfx3_size_add(sizeof(*payload), payload->length,
			     &expected_size) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(expected_size, 1u, &expected_size) !=
			     FB_GFX3_OK) || (payload_size != expected_size) ||
			    (payload->title[payload->length] != '\0'))
				return FB_GFX3_INVALID;
			return FB_GFX3_OK;
		}
	case FB_GFX3_COMMAND_BARRIER:
	case FB_GFX3_COMMAND_PLATFORM_POLL:
	case FB_GFX3_COMMAND_INPUT_POLL:
		return (fb_gfx3_command_payload_size(command) == 0) ?
			FB_GFX3_OK : FB_GFX3_INVALID;
	default:
		return FB_GFX3_INVALID;
	}
}

static int null_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	FB_GFX3_NULL_STATE *state;
	FB_GFX3_COMMAND *command;
	size_t i;
	int result;

	if ((backend == NULL) || (backend->state == NULL) ||
	    (commands == NULL) || (count == 0))
		return FB_GFX3_INVALID;

	state = (FB_GFX3_NULL_STATE *)backend->state;
	for (i = 0; i < count; i++) {
		command = commands[i];
		if ((command == NULL) ||
		    (command->type == FB_GFX3_COMMAND_INVALID) ||
		    (command->type == FB_GFX3_COMMAND_RENDERER_SHUTDOWN) ||
		    (command->size < offsetof(FB_GFX3_COMMAND, payload)) ||
		    (command->size > FB_GFX3_COMMAND_MAX_SIZE) ||
		    (command->sequence <= state->submitted_sequence))
			return FB_GFX3_INVALID;

		result = null_execute_one(state, command);
		if (result != FB_GFX3_OK)
			return result;
		state->submitted_sequence = command->sequence;
	}

	state->completed_sequence = state->submitted_sequence;
	if (submitted_sequence != NULL)
		*submitted_sequence = state->submitted_sequence;
	return FB_GFX3_OK;
}

static uint64_t null_completed_sequence(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_NULL_STATE *state;

	if ((backend == NULL) || (backend->state == NULL))
		return 0;
	state = (FB_GFX3_NULL_STATE *)backend->state;
	return state->completed_sequence;
}

static int null_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	return (sequence <= null_completed_sequence(backend)) ?
		FB_GFX3_OK : FB_GFX3_FAILED;
}

static int null_wait_idle(FB_GFX3_BACKEND *backend)
{
	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	return FB_GFX3_OK;
}

const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_null = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"Null",
	null_probe,
	null_init,
	null_shutdown,
	null_execute,
	null_completed_sequence,
	null_wait_sequence,
	null_wait_idle,
	NULL
};

/* end of gfx3_backend_null.c */
