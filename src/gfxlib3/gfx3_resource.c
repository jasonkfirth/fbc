/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_resource.c

    Purpose:

        Implement generation-tagged resource handles and fence-aware deferred
        destruction for gfxlib3.

    Responsibilities:

        - grow the registry without invalidating handles
        - validate generation, type, state, and reference counts
        - detach retired resources before invoking backend destructors

    This file intentionally does NOT contain:

        - renderer command processing
        - GPU object creation
        - surface synchronization policy
*/

#include "gfx3_resource.h"

/* ------------------------------------------------------------------------- */
/* Handle encoding                                                           */
/* ------------------------------------------------------------------------- */

static FB_GFX3_HANDLE fb_gfx3_make_handle(size_t index, uint32_t generation)
{
	uint64_t low = (uint64_t)(uint32_t)index + 1u;
	return ((uint64_t)generation << 32) | low;
}

static int fb_gfx3_decode_handle(FB_GFX3_HANDLE handle, size_t *index,
	uint32_t *generation)
{
	uint32_t low;

	if ((handle == 0) || (index == NULL) || (generation == NULL))
		return FB_GFX3_INVALID;

	low = (uint32_t)(handle & UINT32_MAX);
	*generation = (uint32_t)(handle >> 32);
	if ((low == 0) || (*generation == 0))
		return FB_GFX3_INVALID;

	*index = (size_t)(low - 1u);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Registry storage                                                          */
/* ------------------------------------------------------------------------- */

static int fb_gfx3_resources_grow(FB_GFX3_RESOURCE_REGISTRY *registry)
{
	FB_GFX3_RESOURCE_SLOT *new_slots;
	size_t old_capacity;
	size_t new_capacity;
	size_t allocation_size;

	old_capacity = registry->capacity;
	if (old_capacity >= (size_t)UINT32_MAX)
		return FB_GFX3_EXHAUSTED;

	if (old_capacity == 0)
		new_capacity = FB_GFX3_RESOURCE_INITIAL_CAPACITY;
	else if (old_capacity > ((size_t)UINT32_MAX / 2u))
		new_capacity = (size_t)UINT32_MAX;
	else
		new_capacity = old_capacity * 2u;

	if (fb_gfx3_size_multiply(new_capacity, sizeof(*new_slots),
	    &allocation_size) != FB_GFX3_OK)
		return FB_GFX3_EXHAUSTED;
	if ((new_capacity == 0u) || (allocation_size == 0u))
		return FB_GFX3_EXHAUSTED;

	new_slots = (FB_GFX3_RESOURCE_SLOT *)realloc(registry->slots,
		allocation_size);
	if (new_slots == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	memset(new_slots + old_capacity, 0,
		(new_capacity - old_capacity) * sizeof(*new_slots));
	registry->slots = new_slots;
	registry->capacity = new_capacity;
	return FB_GFX3_OK;
}

static int fb_gfx3_find_slot(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle, FB_GFX3_RESOURCE_SLOT **slot)
{
	size_t index;
	uint32_t generation;

	if ((registry == NULL) || (slot == NULL) ||
	    (fb_gfx3_decode_handle(handle, &index, &generation) != FB_GFX3_OK) ||
	    (index >= registry->capacity))
		return FB_GFX3_INVALID;

	*slot = &registry->slots[index];
	if (((*slot)->state == FB_GFX3_RESOURCE_FREE) ||
	    ((*slot)->generation != generation))
		return FB_GFX3_INVALID;

	return FB_GFX3_OK;
}

static void fb_gfx3_advance_generation(FB_GFX3_RESOURCE_SLOT *slot)
{
	if (slot->generation == UINT32_MAX)
		slot->generation = 1;
	else
		slot->generation++;
}

/* ------------------------------------------------------------------------- */
/* Public registry operations                                                */
/* ------------------------------------------------------------------------- */

int fb_gfx3_resources_init(FB_GFX3_RESOURCE_REGISTRY *registry,
	size_t initial_capacity)
{
	size_t allocation_size;

	if (registry == NULL)
		return FB_GFX3_INVALID;

	if (initial_capacity == 0)
		initial_capacity = FB_GFX3_RESOURCE_INITIAL_CAPACITY;
	if (initial_capacity > (size_t)UINT32_MAX)
		return FB_GFX3_INVALID;

	if (fb_gfx3_size_multiply(initial_capacity, sizeof(*registry->slots),
	    &allocation_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;

	memset(registry, 0, sizeof(*registry));
	registry->slots = (FB_GFX3_RESOURCE_SLOT *)calloc(1, allocation_size);
	if (registry->slots == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	registry->mutex = fb_MutexCreate();
	if (registry->mutex == NULL) {
		free(registry->slots);
		memset(registry, 0, sizeof(*registry));
		return FB_GFX3_OUT_OF_MEMORY;
	}

	registry->capacity = initial_capacity;
	return FB_GFX3_OK;
}

FB_GFX3_HANDLE fb_gfx3_resource_register(FB_GFX3_RESOURCE_REGISTRY *registry,
	uint32_t type, void *resource, FB_GFX3_RESOURCE_DESTROY destroy)
{
	FB_GFX3_RESOURCE_SLOT *slot;
	FB_GFX3_HANDLE handle = 0;
	size_t i;

	if ((registry == NULL) || (registry->mutex == NULL) ||
	    (type == FB_GFX3_RESOURCE_ANY) || (resource == NULL))
		return 0;

	fb_MutexLock(registry->mutex);
	if (registry->used == registry->capacity) {
		if (fb_gfx3_resources_grow(registry) != FB_GFX3_OK) {
			fb_MutexUnlock(registry->mutex);
			return 0;
		}
	}

	for (i = 0; i < registry->capacity; i++) {
		slot = &registry->slots[i];
		if (slot->state != FB_GFX3_RESOURCE_FREE)
			continue;

		if (slot->generation == 0)
			slot->generation = 1;
		slot->resource = resource;
		slot->destroy = destroy;
		slot->last_use_sequence = 0;
		slot->references = 1;
		slot->type = type;
		slot->state = FB_GFX3_RESOURCE_ACTIVE;
		registry->used++;
		handle = fb_gfx3_make_handle(i, slot->generation);
		break;
	}

	fb_MutexUnlock(registry->mutex);
	return handle;
}

int fb_gfx3_resource_retain(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle, uint32_t expected_type, void **resource)
{
	FB_GFX3_RESOURCE_SLOT *slot;
	int result;

	if ((registry == NULL) || (registry->mutex == NULL))
		return FB_GFX3_INVALID;

	fb_MutexLock(registry->mutex);
	result = fb_gfx3_find_slot(registry, handle, &slot);
	if ((result == FB_GFX3_OK) &&
	    (slot->state != FB_GFX3_RESOURCE_ACTIVE))
		result = FB_GFX3_INVALID;
	if ((result == FB_GFX3_OK) &&
	    (expected_type != FB_GFX3_RESOURCE_ANY) &&
	    (slot->type != expected_type))
		result = FB_GFX3_INVALID;
	if ((result == FB_GFX3_OK) && (slot->references == UINT32_MAX))
		result = FB_GFX3_EXHAUSTED;

	if (result == FB_GFX3_OK) {
		slot->references++;
		if (resource != NULL)
			*resource = slot->resource;
	} else if (resource != NULL) {
		*resource = NULL;
	}

	fb_MutexUnlock(registry->mutex);
	return result;
}

int fb_gfx3_resource_mark_used(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle, uint64_t sequence)
{
	FB_GFX3_RESOURCE_SLOT *slot;
	int result;

	if ((registry == NULL) || (registry->mutex == NULL) || (sequence == 0))
		return FB_GFX3_INVALID;

	fb_MutexLock(registry->mutex);
	result = fb_gfx3_find_slot(registry, handle, &slot);
	if ((result == FB_GFX3_OK) &&
	    (slot->state != FB_GFX3_RESOURCE_ACTIVE))
		result = FB_GFX3_INVALID;
	if ((result == FB_GFX3_OK) &&
	    (sequence > slot->last_use_sequence))
		slot->last_use_sequence = sequence;
	fb_MutexUnlock(registry->mutex);
	return result;
}

int fb_gfx3_resource_release(FB_GFX3_RESOURCE_REGISTRY *registry,
	FB_GFX3_HANDLE handle)
{
	FB_GFX3_RESOURCE_SLOT *slot;
	int result;

	if ((registry == NULL) || (registry->mutex == NULL))
		return FB_GFX3_INVALID;

	fb_MutexLock(registry->mutex);
	result = fb_gfx3_find_slot(registry, handle, &slot);
	if ((result == FB_GFX3_OK) && (slot->references == 0))
		result = FB_GFX3_INVALID;

	if (result == FB_GFX3_OK) {
		slot->references--;
		if (slot->references == 0)
			slot->state = FB_GFX3_RESOURCE_RETIRED;
	}

	fb_MutexUnlock(registry->mutex);
	return result;
}

size_t fb_gfx3_resources_collect(FB_GFX3_RESOURCE_REGISTRY *registry,
	uint64_t completed_sequence)
{
	FB_GFX3_RESOURCE_DESTROY destroy;
	FB_GFX3_RESOURCE_SLOT *slot;
	void *resource;
	size_t collected = 0;
	size_t i;

	if ((registry == NULL) || (registry->mutex == NULL))
		return 0;

	for (;;) {
		destroy = NULL;
		resource = NULL;

		fb_MutexLock(registry->mutex);
		for (i = 0; i < registry->capacity; i++) {
			slot = &registry->slots[i];
			if ((slot->state != FB_GFX3_RESOURCE_RETIRED) ||
			    (slot->last_use_sequence > completed_sequence))
				continue;

			resource = slot->resource;
			destroy = slot->destroy;
			slot->resource = NULL;
			slot->destroy = NULL;
			slot->last_use_sequence = 0;
			slot->references = 0;
			slot->type = FB_GFX3_RESOURCE_ANY;
			slot->state = FB_GFX3_RESOURCE_FREE;
			fb_gfx3_advance_generation(slot);
			registry->used--;
			break;
		}
		fb_MutexUnlock(registry->mutex);

		if (resource == NULL)
			break;
		if (destroy != NULL)
			destroy(resource);
		collected++;
	}

	return collected;
}

void fb_gfx3_resources_destroy(FB_GFX3_RESOURCE_REGISTRY *registry)
{
	FB_GFX3_RESOURCE_DESTROY destroy;
	void *resource;
	size_t i;

	if (registry == NULL)
		return;

	/*
		No API user may access the registry during this call. GPU registries are
		destroyed by the render thread before it releases the backend context.
	*/
	if (registry->slots != NULL) {
		for (i = 0; i < registry->capacity; i++) {
			resource = registry->slots[i].resource;
			destroy = registry->slots[i].destroy;
			registry->slots[i].resource = NULL;
			if ((resource != NULL) && (destroy != NULL))
				destroy(resource);
		}
	}

	if (registry->mutex != NULL)
		fb_MutexDestroy(registry->mutex);
	free(registry->slots);
	memset(registry, 0, sizeof(*registry));
}

/* end of gfx3_resource.c */
