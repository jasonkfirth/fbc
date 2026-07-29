/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_draw_api.c

    Purpose:

        Interpret the historical BASIC DRAW command language above gfxlib3's
        primitive APIs.

    Responsibilities:

        - parse movement, color, scale, angle, paint, and substring commands
        - preserve caller-local DRAW scale and rotation state
        - route line work to GPU primitives or explicit CPU images
        - bound numeric parsing and recursive substring expansion

    This file intentionally does NOT contain:

        - line rasterization
        - the PAINT flood-fill algorithm
        - string glyph rendering
*/

#include "gfx3_api_internal.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_image.h"

#include <ctype.h>
#include <math.h>

enum {
	FB_GFX3_DRAW_RECURSION_LIMIT = 32,
	FB_GFX3_DRAW_PAINT_FILL = 0
};

FBCALL void fb_GfxPaint(void *target, float x, float y,
	unsigned int color, unsigned int border_color, FBSTRING *pattern,
	int paint_mode, int flags);

/* ------------------------------------------------------------------------- */
/* Parser helpers                                                            */
/* ------------------------------------------------------------------------- */

static int draw_parse_number(char **text, int64_t *value)
{
	char *cursor;
	uintmax_t magnitude = 0;
	uintmax_t limit;
	int negative = FALSE;
	int found = FALSE;

	if ((text == NULL) || (*text == NULL) || (value == NULL))
		return FALSE;
	cursor = *text;
	while ((*cursor == ' ') || (*cursor == '\t') ||
	       (*cursor == '+') || (*cursor == '-')) {
		if (*cursor == '-')
			negative = !negative;
		cursor++;
	}
	/*
		DRAW color commands receive the full unsigned 32-bit result of RGB().
		Using intptr_t as the parser limit rejected colors above INT32_MAX in a
		32-bit program even though the command language and renderer accept
		them.  A signed 64-bit intermediate covers every legacy coordinate,
		32-bit color, and pointer value that the old intptr_t parser accepted.
	*/
	limit = (uintmax_t)INT64_MAX + (negative ? 1u : 0u);
	while ((*cursor >= '0') && (*cursor <= '9')) {
		uintmax_t digit = (uintmax_t)(*cursor - '0');

		found = TRUE;
		if ((magnitude > (limit / 10u)) ||
		    ((magnitude == (limit / 10u)) &&
		     (digit > (limit % 10u))))
			return FALSE;
		magnitude = (magnitude * 10u) + digit;
		cursor++;
	}
	if (!found)
		return FALSE;
	*text = cursor;
	if (negative) {
		if (magnitude == (uintmax_t)INT64_MAX + 1u)
			*value = INT64_MIN;
		else
			*value = -(int64_t)magnitude;
	} else {
		*value = (int64_t)magnitude;
	}
	return TRUE;
}

static int draw_modulo_360(int angle)
{
	int result = angle % 360;

	if (result < 0)
		result += 360;
	return result;
}

static float draw_sine(int angle)
{
	switch (draw_modulo_360(angle)) {
	case 0:
	case 180:
		return 0.0f;
	case 90:
		return 1.0f;
	case 270:
		return -1.0f;
	default:
		return sinf((float)angle * 3.14159265358979323846f / 180.0f);
	}
}

static float draw_cosine(int angle)
{
	switch (draw_modulo_360(angle)) {
	case 0:
		return 1.0f;
	case 90:
	case 270:
		return 0.0f;
	case 180:
		return -1.0f;
	default:
		return cosf((float)angle * 3.14159265358979323846f / 180.0f);
	}
}

static uint32_t draw_target_bytes_per_pixel(
	const FB_GFX3_DRAW_STATE *state, void *target,
	const FB_GFX3_IMAGE_VIEW *image)
{
	FB_GFX3_SURFACE *surface;

	if (image != NULL)
		return image->bytes_per_pixel;
	if ((target != NULL) &&
	    (fb_gfx3_gpu_surface_lookup_locked(target, state->mode, &surface) ==
	    FB_GFX3_OK))
		return (surface->depth + 7u) / 8u;
	return (state->mode->depth + 7u) / 8u;
}

/* ------------------------------------------------------------------------- */
/* Primitive routing                                                         */
/* ------------------------------------------------------------------------- */

static int draw_target_line(void *target, FB_GFX3_IMAGE_VIEW *image,
	FB_GFX3_DRAW_STATE *state, float x1, float y1, float x2, float y2)
{
	FB_GFX3_SURFACE *surface;

	if (target == NULL)
		return fb_gfx3_compat_line(state, x1, y1, x2, y2, 0,
			FB_GFX3_LINE_TYPE_LINE, 0xFFFFu,
			FB_GFX3_COORDINATE_AA | FB_GFX3_DEFAULT_COLOR_1);
	if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode, &surface) ==
	    FB_GFX3_OK)
		return fb_gfx3_gpu_surface_line(surface, state, x1, y1, x2, y2,
			0, FB_GFX3_LINE_TYPE_LINE, 0xFFFFu,
			FB_GFX3_COORDINATE_AA | FB_GFX3_DEFAULT_COLOR_1);
	return fb_gfx3_image_line(image, state, x1, y1, x2, y2, 0,
		FB_GFX3_LINE_TYPE_LINE, 0xFFFFu,
		FB_GFX3_COORDINATE_AA | FB_GFX3_DEFAULT_COLOR_1);
}

static int draw_execute(void *target, FB_GFX3_IMAGE_VIEW *image,
	FB_GFX3_DRAW_STATE *state, FBSTRING *command, unsigned int depth)
{
	char *cursor;
	float x;
	float y;
	float x2;
	float y2;
	float delta_x;
	float delta_y;
	float axis_x;
	float axis_y;
	int64_t value1;
	int64_t value2;
	uint32_t saved_flags;
	uint32_t color_mask;
	int angle = 0;
	int diagonal;
	int draw = TRUE;
	int move = TRUE;
	int relative;
	int length;
	int command_letter;
	int result = FB_GFX3_OK;

	if ((state == NULL) || (command == NULL) || (command->data == NULL) ||
	    (depth >= FB_GFX3_DRAW_RECURSION_LIMIT))
		return FB_GFX3_INVALID;
	x = state->last_x;
	y = state->last_y;
	saved_flags = state->flags;
	state->flags |= FB_GFX3_VIEW_SCREEN;
	if (state->mode->depth >= 32)
		color_mask = UINT32_MAX;
	else
		color_mask = (1u << state->mode->depth) - 1u;

	for (cursor = command->data; *cursor != '\0';) {
		command_letter = toupper((unsigned char)*cursor);
		switch (command_letter) {
		case 'B':
			cursor++;
			draw = FALSE;
			break;

		case 'N':
			cursor++;
			move = FALSE;
			break;

		case 'C':
			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			state->foreground_color = fb_gfx3_image_fix_color(
				draw_target_bytes_per_pixel(state, target, image),
				(uint32_t)value1);
			break;

		case 'S':
			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			state->draw_scale = (float)value1 / 4.0f;
			break;

		case 'A':
			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			state->draw_angle = ((int)value1 & 3) * 90;
			break;

		case 'T':
			cursor++;
			if (toupper((unsigned char)*cursor) != 'A') {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			state->draw_angle = draw_modulo_360((int)value1);
			break;

		case 'X': {
			FBSTRING *nested;

			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			nested = (FBSTRING *)(uintptr_t)value1;
			state->last_x = x;
			state->last_y = y;
			result = draw_execute(target, image, state, nested, depth + 1u);
			if (nested != NULL)
				fb_hStrDelTemp(nested);
			if (result != FB_GFX3_OK)
				goto exit_draw;
			x = state->last_x;
			y = state->last_y;
			break;
		}

		case 'P':
			cursor++;
			if (!draw_parse_number(&cursor, &value1)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			value2 = value1;
			if (*cursor == ',') {
				cursor++;
				if (!draw_parse_number(&cursor, &value2)) {
					result = FB_GFX3_INVALID;
					goto exit_draw;
				}
			}
			fb_GfxPaint(target, x, y,
				(uint32_t)value1 & color_mask,
				(uint32_t)value2 & color_mask, NULL,
				FB_GFX3_DRAW_PAINT_FILL, FB_GFX3_COORDINATE_A);
			break;

		case 'M':
			cursor++;
			while ((*cursor == ' ') || (*cursor == '\t'))
				cursor++;
			relative = (*cursor == '+') || (*cursor == '-');
			if (!draw_parse_number(&cursor, &value1) ||
			    (*cursor != ',')) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			cursor++;
			if (!draw_parse_number(&cursor, &value2)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			}
			x2 = (float)value1;
			y2 = (float)value2;
			if (relative) {
				axis_x = draw_cosine(state->draw_angle);
				axis_y = -draw_sine(state->draw_angle);
				delta_x = x2;
				delta_y = y2;
				x2 = (((delta_x * axis_x) -
					(delta_y * axis_y)) * state->draw_scale) + x;
				y2 = (((delta_y * axis_x) +
					(delta_x * axis_y)) * state->draw_scale) + y;
			}
			if (draw)
				result = draw_target_line(target, image, state,
					x, y, x2, y2);
			if (result != FB_GFX3_OK)
				goto exit_draw;
			if (move) {
				x = x2;
				y = y2;
			}
			draw = TRUE;
			move = TRUE;
			break;

		case 'F':
		case 'D':
		case 'G':
		case 'L':
		case 'H':
		case 'U':
		case 'E':
		case 'R':
			diagonal = (command_letter >= 'E') &&
				(command_letter <= 'H');
			switch (command_letter) {
			case 'F':
			case 'D':
				angle = 270;
				break;
			case 'G':
			case 'L':
				angle = 180;
				break;
			case 'H':
			case 'U':
				angle = 90;
				break;
			default:
				angle = 0;
				break;
			}
			cursor++;
			if (!draw_parse_number(&cursor, &value1))
				length = 1;
			else if ((value1 < INT_MIN) || (value1 > INT_MAX)) {
				result = FB_GFX3_INVALID;
				goto exit_draw;
			} else
				length = (int)value1;
			angle = draw_modulo_360(angle + state->draw_angle);
			delta_x = (float)length * state->draw_scale *
				draw_cosine(angle);
			delta_y = (float)length * state->draw_scale *
				-draw_sine(angle);
			if (diagonal) {
				x2 = x + delta_x + delta_y;
				y2 = y + delta_y - delta_x;
			} else {
				x2 = x + delta_x;
				y2 = y + delta_y;
			}
			if (draw)
				result = draw_target_line(target, image, state,
					x, y, x2, y2);
			if (result != FB_GFX3_OK)
				goto exit_draw;
			if (move) {
				x = x2;
				y = y2;
			}
			draw = TRUE;
			move = TRUE;
			break;

		default:
			cursor++;
			break;
		}
	}
	state->last_x = x;
	state->last_y = y;

exit_draw:
	state->flags = saved_flags;
	return result;
}

/* ------------------------------------------------------------------------- */
/* Public DRAW ABI                                                           */
/* ------------------------------------------------------------------------- */

FBCALL void fb_GfxDraw(void *target, FBSTRING *command)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;
	FB_GFX3_IMAGE_VIEW *image_pointer = NULL;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (command != NULL) && (command->data != NULL)) {
		if (target == NULL) {
			draw_execute(target, NULL, state, command, 0);
		} else if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_surface) == FB_GFX3_OK) {
			draw_execute(target, NULL, state, command, 0);
		} else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK) {
			image_pointer = &image;
			draw_execute(target, image_pointer, state, command, 0);
			fb_gfx3_image_cache_metadata_touch(&image);
		}
	}
	if (command != NULL)
		fb_hStrDelTemp(command);
	FB_GRAPHICS_UNLOCK();
}

/* end of gfx3_draw_api.c */
