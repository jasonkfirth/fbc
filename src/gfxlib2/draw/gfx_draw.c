/*
 * FreeBASIC gfxlib2: gfx_draw.c
 * Purpose: Interpret BASIC DRAW strings against the active target.
 * Responsibilities: target preparation, clipping and drawing semantics.
 * This file intentionally does NOT contain native windows or event dispatch.
 */

/* DRAW command */

#include "../fb_gfx.h"
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

/* The ctype API is only defined for EOF or values representable as unsigned char. */
static int draw_toupper(char value)
{
	return toupper((unsigned char)value);
}

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define FB_FALLTHROUGH __attribute__((fallthrough))
#else
#define FB_FALLTHROUGH ((void)0)
#endif

static float base_scale = 1.0;
static int base_angle = 0;

/*
    DRAW coordinates enter integer rasterizers. Validate the rounded or
    truncated value before converting it to int.
*/
static int checked_float_to_int(float value, int round_to_nearest, int *result)
{
	double converted = (double)value;

	if (!isfinite(converted))
		return FALSE;

	converted = round_to_nearest ? round(converted) : trunc(converted);
	if ((converted < (double)INT_MIN) || (converted > (double)INT_MAX))
		return FALSE;

	*result = (int)converted;
	return TRUE;
}

/* Returns sine of angle in degrees - quickly return exact values for 0/90/180/270 */
static float dsin(int angle)
{
	switch( angle )
	{
	case 0:  	return 0.0f;
	case 90: 	return 1.0f;
	case 180:	return 0.0f;
	case 270:	return -1.0f;
	default: 	return sinf((float)((double)(float)angle * PI / 180.0));
	}
}

/* Returns cosine of angle in degrees - quickly return exact values for 0/90/180/270 */
static float dcos(int angle)
{
	switch( angle )
	{
	case 0:  	return 1.0f;
	case 90: 	return 0.0f;
	case 180:	return -1.0f;
	case 270:	return 0.0f;
	default: 	return cosf((float)((double)(float)angle * PI / 180.0));
	}
}

/* Returns a value wrapped to the range 0..359 - avoids '%' in the range 0..719 for speed */
static int mod360(int64_t angle)
{
	if( angle < 0 )
	{
		return (int)(((angle + 1) % 360) + 359);
	}
	else if( angle >= 360 )
	{
		return (int)((angle < 720)? (angle - 360) : (angle % 360));
	}
	else
	{
		return (int)angle;
	}
}

/*
    Accumulate negatively so the most negative intptr_t value can be parsed
    without first forming its unrepresentable positive magnitude.
*/
static int parse_number(char **str, int64_t *value)
{
	char *c = *str;
	int64_t n = 0;
	int64_t limit;
	int64_t multiply_limit;
	int negative = FALSE;
	int has_digits = FALSE;
	int too_large = FALSE;

	while ((*c == ' ') || (*c == '\t') || (*c == '+') || (*c == '-'))
	{
		if (*c == '-')
			negative = !negative;
		c++;
	}
	limit = negative ? INT64_MIN : -INT64_MAX;
	multiply_limit = limit / 10;
	while ((*c >= '0') && (*c <= '9')) {
		int64_t digit = (int64_t)(*c - '0');
		has_digits = TRUE;
		if (!too_large) {
			if (n < multiply_limit) {
				too_large = TRUE;
			}
			else {
				n *= 10;
				if (n < (limit + digit))
					too_large = TRUE;
				else
					n -= digit;
			}
		}
		c++;
	}
	*str = c;

	if (too_large)
		return -1;
	if (!has_digits)
		return 0;

	*value = negative ? n : -n;
	return 1;
}

FBCALL void fb_GfxDraw(void *target, FBSTRING_ACCESS *command)
{
	FB_GFXCTX *context;
	float x, y, dx, dy, ax, ay, x2, y2;
	int angle = 0, diagonal = FALSE;
	char *c;
	int64_t value1, value2;
	int draw = TRUE, move = TRUE, length = 0, flags, rel, parse_result;
	int draw_x1, draw_y1, draw_x2, draw_y2;

	FB_GRAPHICS_LOCK( );

	if ((!fb_gfx) || (!command) || (!command->data)) {
		if (command)
			fb_hStrDelTemp(command);
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);
	fb_hSetPixelTransfer(context, MASK_A_32);

	x = context->last_x;
	y = context->last_y;

	DRIVER_LOCK();

	flags = context->flags;
	context->flags |= CTX_VIEW_SCREEN;

	for (c = command->data; *c;) {
		switch (draw_toupper(*c)) {
			case 'B':
				c++;
				draw = FALSE;
				break;

			case 'N':
				c++;
				move = FALSE;
				break;

			case 'C':
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				context->fg_color = fb_hFixColor(context->target_bpp, (unsigned int)value1);
				break;

			case 'S':
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				base_scale = (float)((double)(float)value1 / 4.0);
				break;

			case 'A':
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				base_angle = (value1 & 0x3) * 90;
				break;

			case 'T':
				c++;
				if (draw_toupper(*c) != 'A')
					goto error;
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				base_angle = mod360( value1 );
				break;

			case 'X':
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;

				/* Store our current x/y for the recursive fb_GfxDraw() call */
				context->last_x = x;
				context->last_y = y;

				DRIVER_UNLOCK();
				fb_GfxDraw(target, (FBSTRING_ACCESS *)(uintptr_t)value1);
				DRIVER_LOCK();

				/* And update to x/y produced by the recursive fb_GfxDraw() call */
				x = context->last_x;
				y = context->last_y;

				break;

			case 'P':
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				value2 = value1;
				if (*c == ',') {
					c++;
					parse_result = parse_number(&c, &value2);
					if (parse_result != 1)
						goto error;
				}
				DRIVER_UNLOCK();
				fb_GfxPaint(target, x, y,
				            (unsigned int)((uint64_t)value1 & (uint64_t)fb_gfx->color_mask),
				            (unsigned int)((uint64_t)value2 & (uint64_t)fb_gfx->color_mask),
				            NULL, PAINT_TYPE_FILL, COORD_TYPE_A);
				DRIVER_LOCK();
				break;

			case 'M':
				c++;
				while ((*c == ' ') || (*c == '\t'))
					c++;
				rel = ((*c == '+') || (*c == '-'));
				parse_result = parse_number(&c, &value1);
				if (parse_result != 1)
					goto error;
				if (*c++ != ',')
					goto error;
				parse_result = parse_number(&c, &value2);
				if (parse_result != 1)
					goto error;
				x2 = (float)value1;
				y2 = (float)value2;
				if (rel) {
					ax = dcos(base_angle);
					ay = -dsin(base_angle);
					dx = x2;
					dy = y2;
					x2 = (((dx * ax) - (dy * ay)) * base_scale) + x;
					y2 = (((dy * ax) + (dx * ay)) * base_scale) + y;
				}
				if (draw) {
					if (!checked_float_to_int(x, FALSE, &draw_x1) ||
					    !checked_float_to_int(y, FALSE, &draw_y1) ||
					    !checked_float_to_int(x2, FALSE, &draw_x2) ||
					    !checked_float_to_int(y2, FALSE, &draw_y2))
						goto error;
					DRIVER_UNLOCK();
					fb_GfxLine(target,
					           (float)draw_x1, (float)draw_y1,
					           (float)draw_x2, (float)draw_y2,
					           0, LINE_TYPE_LINE, 0xFFFFu,
					           (int)(COORD_TYPE_AA | DEFAULT_COLOR_1));
					DRIVER_LOCK();
				}
				if (move) {
					x = x2;
					y = y2;
				}
				move = draw = TRUE;
				break;

			case 'F': case 'D': angle += 90; FB_FALLTHROUGH;
			case 'G': case 'L': angle += 90; FB_FALLTHROUGH;
			case 'H': case 'U': angle += 90; FB_FALLTHROUGH;
			case 'E': case 'R':
				diagonal = ((draw_toupper(*c) >= 'E') && (draw_toupper(*c) <= 'H'));
				c++;
				parse_result = parse_number(&c, &value1);
				if (parse_result < 0)
					goto error;
				if (parse_result == 1) {
					/* The integer rasterizer cannot represent a wider length. */
					if ((value1 < INT_MIN) || (value1 > INT_MAX))
						goto error;
					length = (int)value1;
				}
				else {
					length = 1;
				}

				angle = mod360( angle + base_angle );
				dx = (float)length * base_scale * dcos( angle );
				dy = (float)length * base_scale * -dsin( angle );

				if (diagonal) {
					x2 = x + (dx + dy);
					y2 = y + (dy - dx);
				}
				else {
					x2 = x + dx;
					y2 = y + dy;
				}
				if (draw) {
					if (!checked_float_to_int(x, TRUE, &draw_x1) ||
					    !checked_float_to_int(y, TRUE, &draw_y1) ||
					    !checked_float_to_int(x2, TRUE, &draw_x2) ||
					    !checked_float_to_int(y2, TRUE, &draw_y2))
						goto error;
					fb_GfxDrawLine(context, draw_x1, draw_y1,
					               draw_x2, draw_y2, context->fg_color,
					               0xffffu);
				}
				if (move) {
					x = x2;
					y = y2;
				}

				angle = 0;
				move = draw = TRUE;
				break;

			default:
				c++;
				break;
		}
	}

	context->last_x = x;
	context->last_y = y;

error:
	context->flags = flags;

	DRIVER_UNLOCK();

	/* del if temp */
	fb_hStrDelTemp( command );

	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_draw.c */
