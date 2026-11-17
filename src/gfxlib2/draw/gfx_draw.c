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

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define FB_FALLTHROUGH __attribute__((fallthrough))
#else
#define FB_FALLTHROUGH ((void)0)
#endif

#define FB_NAN		((intptr_t)0x80000000)

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
static int mod360(intptr_t angle)
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
static intptr_t parse_number(char **str, int *overflow)
{
	char *c = *str;
	intptr_t n = 0;
	intptr_t limit;
	intptr_t multiply_limit;
	int negative = FALSE;
	int has_digits = FALSE;
	int too_large = FALSE;

	while ((*c == ' ') || (*c == '\t') || (*c == '+') || (*c == '-'))
	{
		if (*c == '-')
			negative = !negative;
		c++;
	}
	limit = negative ? INTPTR_MIN : -INTPTR_MAX;
	multiply_limit = limit / 10;
	while ((*c >= '0') && (*c <= '9')) {
		intptr_t digit = (intptr_t)(*c - '0');
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
	*overflow = too_large;

	if (!has_digits || too_large)
		return FB_NAN;
	return negative ? n : -n;
}

FBCALL void fb_GfxDraw(void *target, FBSTRING_ACCESS *command)
{
	FB_GFXCTX *context;
	float x, y, dx, dy, ax, ay, x2, y2;
	int angle = 0, diagonal = FALSE;
	char *c;
	intptr_t value1, value2;
	int draw = TRUE, move = TRUE, length = 0, flags, rel, parse_overflow;
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
		switch (toupper(*c)) {
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
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				context->fg_color = fb_hFixColor(context->target_bpp, (unsigned int)value1);
				break;

			case 'S':
				c++;
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				base_scale = (float)((double)(float)value1 / 4.0);
				break;

			case 'A':
				c++;
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				base_angle = (value1 & 0x3) * 90;
				break;

			case 'T':
				c++;
				if (toupper(*c) != 'A')
					goto error;
				c++;
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				base_angle = mod360( value1 );
				break;

			case 'X':
				c++;
				/* Here we could be more severe with checking, but it's unlikely our substring
				 * resides at location FB_NAN (0x80000000) */
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;

				/* Store our current x/y for the recursive fb_GfxDraw() call */
				context->last_x = x;
				context->last_y = y;

				DRIVER_UNLOCK();
				fb_GfxDraw(target, (FBSTRING_ACCESS *)value1);
				DRIVER_LOCK();

				/* And update to x/y produced by the recursive fb_GfxDraw() call */
				x = context->last_x;
				y = context->last_y;

				break;

			case 'P':
				c++;
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				value2 = value1;
				if (*c == ',') {
					c++;
					value2 = parse_number(&c, &parse_overflow);
					if ((value2 == FB_NAN) || parse_overflow)
						goto error;
				}
				DRIVER_UNLOCK();
				fb_GfxPaint(target, x, y,
				            (unsigned int)(value1 & fb_gfx->color_mask),
				            (unsigned int)(value2 & fb_gfx->color_mask),
				            NULL, PAINT_TYPE_FILL, COORD_TYPE_A);
				DRIVER_LOCK();
				break;

			case 'M':
				c++;
				while ((*c == ' ') || (*c == '\t'))
					c++;
				rel = ((*c == '+') || (*c == '-'));
				value1 = parse_number(&c, &parse_overflow);
				if ((value1 == FB_NAN) || parse_overflow)
					goto error;
				if (*c++ != ',')
					goto error;
				value2 = parse_number(&c, &parse_overflow);
				if ((value2 == FB_NAN) || parse_overflow)
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
				diagonal = ((toupper(*c) >= 'E') && (toupper(*c) <= 'H'));
				c++;
				value1 = parse_number(&c, &parse_overflow);
				if (parse_overflow)
					goto error;
				if (value1 != FB_NAN) {
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
