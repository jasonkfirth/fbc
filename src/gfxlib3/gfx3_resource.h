/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_resource.h

    Purpose:

        Define the generation-tagged registry used for GPU and staging
        resources shared by queued commands.

    Responsibilities:

        - reject stale or type-confused handles
        - reference-count resources across API and render threads
        - delay destruction until the last GPU sequence has completed

    This file intentionally does NOT contain:

        - backend resource structures
        - FB.IMAGE layout handling
        - GPU fence polling
*/

#ifndef __FB_GFX3_RESOURCE_H__
#define __FB_GFX3_RESOURCE_H__

#include "fb_gfx3.h"

#define FB_GFX3_RESOURCE_ANY 0u
#define FB_GFX3_RESOURCE_INITIAL_CAPACITY 64u

enum FB_GFX3_RESOURCE_STATE {
	FB_GFX3_RESOURCE_FREE = 0,
	FB_GFX3_RESOURCE_ACTIVE,
	FB_GFX3_RESOURCE_RETIRED
};

typedef void (*FB_GFX3_RESOURCE_DESTROY)(void *resource);

typedef struct FB_GFX3_RESOURCE_SLOT {
	void *resource;
	FB_GFX3_RESOURCE_DESTROY destroy;
	uint64_t last_use_sequence;
	uint32_t generation;
	uint32_t references;
	uint32_t type;
	uint32_t state;
} FB_GFX3_RESOURCE_SLOT;

typedef struct FB_GFX3_RESOURCE_REGISTRY {
	FBMUTEX *mutex;
	FB_GFX3_RESOURCE_SLOT *slots;
	size_t capacity;
	size_t used;
} FB_GFX3_RESOURCE_REGISTRY;

int fb_gfx3_resources_init(FB_GFX3_RESOURCE_REGISTRY *registry,
	size_t initial_capacity);
FB_GFX3_HANDLE fb_gfx3_resource_register(FB_GFX3_RESOURCE_REGISTRY *registry,
	uint32_t type, void *resource, FB_GFX3_RESOURCE_DESTROY destroy);
int fb_gfx3_resource_retain(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle, uint32_t expected_type, void **resource);
int fb_gfx3_resource_mark_used(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle, uint64_t sequence);
int fb_gfx3_resource_release(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle);
size_t fb_gfx3_resources_collect(FB_GFX3_RESOURCE_REGISTRY *registry,
	uint64_t completed_sequence);
void fb_gfx3_resources_destroy(FB_GFX3_RESOURCE_REGISTRY *registry);

#endif

/* end of gfx3_resource.h */
