/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend.h

    Purpose:

        Define the private contract between the common render thread and a
        Vulkan, OpenGL, or null renderer backend.

    Responsibilities:

        - report concrete backend capabilities
        - initialize and shut down renderer-owned backend state
        - execute ordered batches and expose completion progress

    This file intentionally does NOT contain:

        - Vulkan or OpenGL declarations
        - platform window-system handles
        - FreeBASIC public API declarations
*/

#ifndef __FB_GFX3_BACKEND_H__
#define __FB_GFX3_BACKEND_H__

#include "gfx3_command.h"

#define FB_GFX3_BACKEND_ABI_VERSION 1u
#define FB_GFX3_BACKEND_GL_EXTENSIONS_CAPACITY 16384u

struct FB_GFX3_RESOURCE_REGISTRY;
struct FB_GFX3_LOGGER;

enum FB_GFX3_BACKEND_FEATURE {
	FB_GFX3_FEATURE_INDEXED_SURFACES = 0x00000001u,
	FB_GFX3_FEATURE_COMPUTE = 0x00000002u,
	FB_GFX3_FEATURE_STORAGE_IMAGES = 0x00000004u,
	FB_GFX3_FEATURE_TIMELINE_FENCES = 0x00000008u,
	FB_GFX3_FEATURE_PRESENT_MAILBOX = 0x00000010u,
	FB_GFX3_FEATURE_PRESENT_IMMEDIATE = 0x00000020u,
	FB_GFX3_FEATURE_DEBUG_OUTPUT = 0x00000040u,
	/* Backend consumes one ordered command containing many ordinary PUTs. */
	FB_GFX3_FEATURE_PACKED_BLITS = 0x00000080u,
	/*
		The packed PUT command may retain different source surfaces in one FIFO
		stream. Backends without this bit continue receiving one-source packets.
	*/
	FB_GFX3_FEATURE_HETEROGENEOUS_BLITS = 0x00000100u,
	/*
		The backend accepts filled and outline rectangles in one FIFO packet.
		This is separate from COMPUTE because older compute backends only accept
		opaque filled rectangles in their packed path.
	*/
	FB_GFX3_FEATURE_PACKED_RECTANGLES = 0x00000200u,
	/* Packed outline rectangles preserve non-solid 16-bit style patterns. */
	FB_GFX3_FEATURE_PACKED_STYLED_RECTANGLES = 0x00000400u,
	/*
		The backend consumes one ordered packet containing opaque LINE
		operations. This avoids one allocation and queue record per scanline in
		software-style polygon fillers while retaining GPU rasterization.
	*/
	FB_GFX3_FEATURE_PACKED_LINES = 0x00000800u
};

typedef struct FB_GFX3_BACKEND_CAPS {
	uint32_t abi_version;
	uint32_t features;
	uint32_t max_surface_width;
	uint32_t max_surface_height;
	uint32_t max_batch_commands;
	/* Maximum operations accepted by one FB_GFX3_COMMAND_BLITS payload. */
	uint32_t max_packed_blits;
	uint32_t reserved[10];
} FB_GFX3_BACKEND_CAPS;

/*
	OpenGL contexts belong exclusively to the render thread.  This immutable
	snapshot is filled while that thread owns a newly created context, then read
	by SCREENCONTROL after normal renderer startup synchronization.  BASIC code
	never receives the live context or is allowed to issue unsynchronized GL.
*/
typedef struct FB_GFX3_BACKEND_GL_INFO {
	uint32_t available;
	uint32_t color_bits;
	uint32_t color_red_bits;
	uint32_t color_green_bits;
	uint32_t color_blue_bits;
	uint32_t color_alpha_bits;
	uint32_t depth_bits;
	uint32_t stencil_bits;
	uint32_t accum_bits;
	uint32_t accum_red_bits;
	uint32_t accum_green_bits;
	uint32_t accum_blue_bits;
	uint32_t accum_alpha_bits;
	uint32_t samples;
	char extensions[FB_GFX3_BACKEND_GL_EXTENSIONS_CAPACITY];
} FB_GFX3_BACKEND_GL_INFO;

typedef struct FB_GFX3_BACKEND_CONFIG {
	void *platform;
	struct FB_GFX3_RESOURCE_REGISTRY *resources;
	struct FB_GFX3_LOGGER *logger;
	const char *title;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t page_count;
	uint32_t flags;
	uint32_t reserved[6];
} FB_GFX3_BACKEND_CONFIG;

typedef struct FB_GFX3_BACKEND {
	void *state;
	FB_GFX3_BACKEND_CAPS caps;
	FB_GFX3_BACKEND_GL_INFO gl_info;
} FB_GFX3_BACKEND;

typedef struct FB_GFX3_BACKEND_VTABLE {
	uint32_t abi_version;
	const char *name;
	int (*probe)(FB_GFX3_BACKEND_CAPS *caps);
	int (*init)(FB_GFX3_BACKEND *backend,
		const FB_GFX3_BACKEND_CONFIG *config);
	void (*shutdown)(FB_GFX3_BACKEND *backend);
	int (*execute)(FB_GFX3_BACKEND *backend,
		FB_GFX3_COMMAND *const *commands, size_t count,
		uint64_t *submitted_sequence);
	uint64_t (*completed_sequence)(FB_GFX3_BACKEND *backend);
	int (*wait_sequence)(FB_GFX3_BACKEND *backend, uint64_t sequence);
	int (*wait_idle)(FB_GFX3_BACKEND *backend);
	/* Valid only while a gfxlib3 interop callback owns this render thread. */
	void *(*get_opengl_proc)(FB_GFX3_BACKEND *backend, const char *name);
} FB_GFX3_BACKEND_VTABLE;

#endif

/* end of gfx3_backend.h */
