/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_protocol.h

    Purpose:

        Define backend-independent command payloads produced by the
        compatibility front end and consumed by renderer backends.

    Responsibilities:

        - use fixed-width fields for copied command data
		- define surface, clipping, point, line, box, and readback payloads
        - keep variable payload counts explicitly bounds-checkable

    This file intentionally does NOT contain:

        - queue headers or sequence numbers
        - backend GPU objects
        - FreeBASIC parser state
*/

#ifndef __FB_GFX3_PROTOCOL_H__
#define __FB_GFX3_PROTOCOL_H__

#include "gfx3_command.h"

enum FB_GFX3_RESOURCE_TYPE {
	FB_GFX3_RESOURCE_SURFACE = 1
};

enum FB_GFX3_SURFACE_USAGE {
	FB_GFX3_SURFACE_RENDER_TARGET = 0x00000001u,
	FB_GFX3_SURFACE_SAMPLED = 0x00000002u,
	FB_GFX3_SURFACE_TRANSFER_SOURCE = 0x00000004u,
	FB_GFX3_SURFACE_TRANSFER_DESTINATION = 0x00000008u,
	FB_GFX3_SURFACE_CPU_VISIBLE = 0x00000010u
};

/* These values intentionally match the existing gfxlib2 PUT mode numbers. */
enum FB_GFX3_BLIT_MODE {
	FB_GFX3_BLIT_TRANS = 0,
	FB_GFX3_BLIT_PSET = 1,
	FB_GFX3_BLIT_PRESET = 2,
	FB_GFX3_BLIT_AND = 3,
	FB_GFX3_BLIT_OR = 4,
	FB_GFX3_BLIT_XOR = 5,
	FB_GFX3_BLIT_ALPHA = 6,
	FB_GFX3_BLIT_ADD = 7,
	FB_GFX3_BLIT_CUSTOM = 8,
	FB_GFX3_BLIT_BLEND = 9
};

enum FB_GFX3_PRIMITIVE_FLAGS {
	FB_GFX3_PRIMITIVE_ALPHA_BLEND = 0x00000001u
};

/*
	gfxlib2 alpha primitive transfer

	This deliberately differs from PUT ALPHA.  Primitive RGB channels use the
	source alpha as an integer interpolation factor divided by 256, while the
	stored alpha channel is replaced by the source alpha.  Callers only request
	this operation for 32-bit targets and source alpha values below 255.
*/
static inline uint32_t fb_gfx3_alpha_primitive_pixel(uint32_t source,
	uint32_t destination)
{
	uint32_t source_red_blue = source & 0x00FF00FFu;
	uint32_t source_green = source & 0x0000FF00u;
	uint32_t destination_red_blue = destination & 0x00FF00FFu;
	uint32_t destination_green = destination & 0x0000FF00u;
	uint32_t alpha = source >> 24;

	source_red_blue = ((source_red_blue - destination_red_blue) * alpha) >> 8;
	source_green = ((source_green - destination_green) * alpha) >> 8;
	return ((destination_red_blue + source_red_blue) & 0x00FF00FFu) |
		((destination_green + source_green) & 0x0000FF00u) |
		(source & 0xFF000000u);
}

typedef struct FB_GFX3_RECT {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
} FB_GFX3_RECT;

typedef struct FB_GFX3_SURFACE_CREATE_COMMAND {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t usage;
	uint32_t clear_color;
	uint32_t reserved[3];
} FB_GFX3_SURFACE_CREATE_COMMAND;

typedef struct FB_GFX3_SURFACE_UPLOAD_COMMAND {
	int32_t destination_x;
	int32_t destination_y;
	uint32_t width;
	uint32_t height;
	uint32_t source_pitch;
	uint32_t data_size;
	uint32_t reserved[2];
	unsigned char data[];
} FB_GFX3_SURFACE_UPLOAD_COMMAND;

typedef struct FB_GFX3_SURFACE_DOWNLOAD_COMMAND {
	int32_t source_x;
	int32_t source_y;
	uint32_t width;
	uint32_t height;
	uint32_t destination_pitch;
	uint32_t destination_size;
	uint64_t destination_address;
} FB_GFX3_SURFACE_DOWNLOAD_COMMAND;

typedef struct FB_GFX3_CLEAR_COMMAND {
	FB_GFX3_RECT clip;
	uint32_t color;
	uint32_t flags;
	uint32_t reserved[2];
} FB_GFX3_CLEAR_COMMAND;

typedef struct FB_GFX3_POINT {
	int32_t x;
	int32_t y;
	uint32_t color;
	uint32_t flags;
} FB_GFX3_POINT;

typedef struct FB_GFX3_POINTS_COMMAND {
	FB_GFX3_RECT clip;
	uint32_t count;
	uint32_t reserved[3];
	FB_GFX3_POINT point[];
} FB_GFX3_POINTS_COMMAND;

enum FB_GFX3_GLYPH_FLAGS {
	/* Write the command's background colour where a glyph row bit is clear. */
	FB_GFX3_GLYPH_BACKGROUND = 0x00000001u
};

/*
	One fixed-width record describes an 8-pixel-wide bitmap glyph.  Sixteen rows
	cover every built-in console font while keeping the std430 layout identical
	on the C and shader sides.  Copied row masks make deferred commands independent
	of the lifetime of the canonical font table or caller string.
*/
typedef struct FB_GFX3_GLYPH {
	int32_t x;
	int32_t y;
	uint32_t foreground;
	uint32_t background;
	uint32_t width;
	uint32_t height;
	uint32_t flags;
	uint32_t reserved;
	uint32_t row[16];
} FB_GFX3_GLYPH;

typedef struct FB_GFX3_GLYPHS_COMMAND {
	FB_GFX3_RECT clip;
	uint32_t count;
	uint32_t reserved[3];
	FB_GFX3_GLYPH glyph[];
} FB_GFX3_GLYPHS_COMMAND;

typedef struct FB_GFX3_LINE_COMMAND {
	FB_GFX3_RECT clip;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t style;
	uint32_t flags;
	uint32_t reserved;
} FB_GFX3_LINE_COMMAND;

/*
	All records in a LINES packet have the same target, but retain their own clip
	and style so the packet has exactly the semantics of its original FIFO LINE
	commands. Keeping the ordinary record layout also lets backends share their
	established line validation and shader input conversion.
*/
typedef struct FB_GFX3_LINES_COMMAND {
	uint32_t count;
	uint32_t reserved[3];
	FB_GFX3_LINE_COMMAND line[];
} FB_GFX3_LINES_COMMAND;

typedef struct FB_GFX3_RECTANGLE_COMMAND {
	FB_GFX3_RECT clip;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t style;
	uint32_t filled;
	uint32_t flags;
} FB_GFX3_RECTANGLE_COMMAND;

/*
	The context emits this only for consecutive opaque filled rectangles on one
	target. Each item retains its own clip and colour, so the backend preserves
	the same drawing order as the individual public LINE ... , BF operations.
*/
typedef struct FB_GFX3_RECTANGLES_COMMAND {
	uint32_t count;
	uint32_t reserved[3];
	FB_GFX3_RECTANGLE_COMMAND rectangle[];
} FB_GFX3_RECTANGLES_COMMAND;

typedef struct FB_GFX3_ELLIPSE_COMMAND {
	FB_GFX3_RECT clip;
	int32_t center_x;
	int32_t center_y;
	float radius_x;
	float radius_y;
	uint32_t color;
	uint32_t filled;
	uint32_t flags;
	uint32_t reserved;
} FB_GFX3_ELLIPSE_COMMAND;

/*
	GPU PAINT receives already resolved colours and a copied 8 by 8 pattern.
	This command's clip is the complete flood domain, not merely a write clip:
	pixels outside it are borders. The fixed representation makes it safe for
	the deferred render thread to outlive a temporary BASIC pattern string.
*/
typedef struct FB_GFX3_PAINT_COMMAND {
	FB_GFX3_RECT clip;
	int32_t x;
	int32_t y;
	uint32_t color;
	uint32_t border_color;
	uint32_t flags;
	uint32_t paint_mode;
	uint32_t pattern_size;
	uint32_t pattern_origin_x;
	uint32_t pattern_origin_y;
	/* 8 by 8 pixels at the widest supported 32-bit source layout. */
	uint32_t pattern_word[64];
} FB_GFX3_PAINT_COMMAND;

typedef struct FB_GFX3_BLIT_COMMAND {
	FB_GFX3_HANDLE source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int32_t destination_x;
	int32_t destination_y;
	uint32_t mode;
	uint32_t alpha;
	uint32_t reserved[2];
} FB_GFX3_BLIT_COMMAND;

/*
	The individual records stay ordered in the payload, so overlap semantics are
	identical to the equivalent FIFO sequence of BLIT commands. The header
	repeats the first record's source, mode, and alpha for backends which expose
	only FB_GFX3_FEATURE_PACKED_BLITS. A backend advertising heterogeneous blits
	must read those values from every record.
*/
typedef struct FB_GFX3_BLITS_COMMAND {
	FB_GFX3_HANDLE source;
	uint32_t mode;
	uint32_t alpha;
	uint32_t count;
	uint32_t reserved[3];
	FB_GFX3_BLIT_COMMAND blit[];
} FB_GFX3_BLITS_COMMAND;

enum FB_GFX3_TRANSFORM_FILTER {
	FB_GFX3_TRANSFORM_FILTER_NEAREST = 0,
	FB_GFX3_TRANSFORM_FILTER_LINEAR = 1
};

enum FB_GFX3_TRANSFORM_WRAP {
	FB_GFX3_TRANSFORM_WRAP_CLAMP = 0,
	FB_GFX3_TRANSFORM_WRAP_REPEAT = 1
};

/*
	The row-major matrix maps a destination pixel centre to source image
	coordinates. The first two rows produce the source numerator and the third
	row produces the projective denominator. An affine operation uses 0, 0, 1
	for the third row. Mode 7 uses destination y minus its horizon there.
*/
typedef struct FB_GFX3_TRANSFORM_BLIT_COMMAND {
	FB_GFX3_HANDLE source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	FB_GFX3_RECT destination_bounds;
	float inverse[9];
	uint32_t mode;
	uint32_t alpha;
	uint32_t filter;
	uint32_t wrap;
	uint32_t reserved[3];
} FB_GFX3_TRANSFORM_BLIT_COMMAND;

typedef struct FB_GFX3_READ_PIXEL_COMMAND {
	int32_t x;
	int32_t y;
	uint32_t reserved[2];
} FB_GFX3_READ_PIXEL_COMMAND;

typedef struct FB_GFX3_PALETTE_COMMAND {
	uint32_t color[256];
} FB_GFX3_PALETTE_COMMAND;

typedef struct FB_GFX3_PAGE_SET_COMMAND {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t reserved;
} FB_GFX3_PAGE_SET_COMMAND;

typedef struct FB_GFX3_WINDOW_TITLE_COMMAND {
	uint32_t length;
	uint32_t reserved;
	char title[];
} FB_GFX3_WINDOW_TITLE_COMMAND;

/*
	Interop callbacks run only on gfxlib3's render thread.  Both addresses are
	owned by the caller and are deliberately not dereferenced until execution,
	which keeps command submission independent of caller-local pointer sizes.
*/
typedef void (FBCALL *FB_GFX3_INTEROP_CALLBACK)(void *user_data);

typedef struct FB_GFX3_INTEROP_CALLBACK_COMMAND {
	uintptr_t callback;
	uintptr_t user_data;
} FB_GFX3_INTEROP_CALLBACK_COMMAND;

#endif

/* end of gfx3_protocol.h */
