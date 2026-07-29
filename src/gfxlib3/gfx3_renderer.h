/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_renderer.h

    Purpose:

        Define the common render-thread owner that connects API submissions,
        the command queue, the resource registry, and one renderer backend.

    Responsibilities:

        - create and join the dedicated render thread
        - initialize and shut down the backend on that thread
        - transfer command ownership after successful submission
        - wake synchronous callers after execution or renderer failure

    This file intentionally does NOT contain:

        - FreeBASIC graphics entry points
        - primitive command payload definitions
        - window-system event handling
*/

#ifndef __FB_GFX3_RENDERER_H__
#define __FB_GFX3_RENDERER_H__

#include "gfx3_backend.h"
#include "gfx3_resource.h"

#include <stdatomic.h>

#define FB_GFX3_DEFAULT_QUEUE_CAPACITY 1024u

typedef struct FB_GFX3_RENDERER_CONFIG {
	const FB_GFX3_BACKEND_VTABLE *backend;
	FB_GFX3_BACKEND_CONFIG backend_config;
	size_t queue_capacity;
	size_t resource_capacity;
	uint32_t idle_poll_milliseconds;
} FB_GFX3_RENDERER_CONFIG;

typedef struct FB_GFX3_RENDERER {
	FB_GFX3_COMMAND_QUEUE queue;
	FB_GFX3_RESOURCE_REGISTRY resources;
	const FB_GFX3_BACKEND_VTABLE *backend_vtable;
	FB_GFX3_BACKEND backend;
	FB_GFX3_BACKEND_CONFIG backend_config;
	FB_GFX3_COMPLETION startup;
	FBTHREAD *thread;
	FBTHREAD *idle_pump_thread;
	_Atomic int idle_pump_stop;
	/*
		The idle pump samples this counter before sleeping. Normal renderer work
		already processes platform events, so a changed counter suppresses the
		redundant periodic PLATFORM_POLL command for that interval.
	*/
	_Atomic uint64_t activity_epoch;
	uint32_t idle_poll_milliseconds;
	int backend_initialized;
} FB_GFX3_RENDERER;

int fb_gfx3_renderer_init(FB_GFX3_RENDERER *renderer,
	const FB_GFX3_RENDERER_CONFIG *config);
int fb_gfx3_renderer_submit(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *command, uint64_t *sequence);
int fb_gfx3_renderer_submit_many(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *const *commands, size_t count, uint64_t *sequence);
int fb_gfx3_renderer_shutdown(FB_GFX3_RENDERER *renderer);

/*
	SCREENGLPROC is intentionally available only while this render thread is
	executing an explicit interop callback.  Returning a live procedure pointer
	to ordinary BASIC code would invite calls from the wrong thread.
*/
void *fb_gfx3_renderer_callback_gl_proc(const char *name);

#endif

/* end of gfx3_renderer.h */
