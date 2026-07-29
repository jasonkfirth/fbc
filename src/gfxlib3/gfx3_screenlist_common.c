/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_screenlist_common.c

    Purpose:

        Collect, sort, and deduplicate packed SCREENLIST display modes.

    Responsibilities:

        - apply gfxlib2-compatible display-depth equivalence
        - grow packed-mode storage with checked arithmetic
        - sort and deduplicate completed native mode lists

    This file intentionally does NOT contain:

        - native display-system calls
        - public iterator state
        - fallback lists of standard resolutions
*/

#include "gfx3_screenlist_internal.h"

static int screenlist_mode_compare(const void *left, const void *right)
{
	int left_mode = *(const int *)left;
	int right_mode = *(const int *)right;
	unsigned int left_width = ((unsigned int)left_mode >> 16) & 0xFFFFu;
	unsigned int right_width = ((unsigned int)right_mode >> 16) & 0xFFFFu;
	unsigned int left_height = (unsigned int)left_mode & 0xFFFFu;
	unsigned int right_height = (unsigned int)right_mode & 0xFFFFu;

	if (left_width != right_width)
		return (left_width < right_width) ? -1 : 1;
	if (left_height != right_height)
		return (left_height < right_height) ? -1 : 1;
	return 0;
}

int fb_gfx3_screenlist_depth_matches(uint32_t native_depth, int depth)
{
	return (native_depth == (uint32_t)depth) ||
		((native_depth == 15u) && (depth == 16)) ||
		((native_depth == 16u) && (depth == 15)) ||
		((native_depth == 24u) && (depth == 32)) ||
		((native_depth == 32u) && (depth == 24));
}

int fb_gfx3_screenlist_append(int **modes, size_t *count, size_t *capacity,
	int mode)
{
	int *new_modes;
	size_t new_capacity;
	size_t allocation_size;

	if ((modes == NULL) || (count == NULL) || (capacity == NULL))
		return FB_GFX3_INVALID;
	if (*count == *capacity) {
		new_capacity = (*capacity == 0) ? 16u : (*capacity * 2u);
		if ((new_capacity < *capacity) ||
		    (fb_gfx3_size_multiply(new_capacity, sizeof(**modes),
		     &allocation_size) != FB_GFX3_OK))
			return FB_GFX3_OUT_OF_MEMORY;
		new_modes = (int *)realloc(*modes, allocation_size);
		if (new_modes == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		*modes = new_modes;
		*capacity = new_capacity;
	}
	(*modes)[(*count)++] = mode;
	return FB_GFX3_OK;
}

int fb_gfx3_screenlist_finish(int *modes, size_t count,
	int **result_modes, size_t *result_count)
{
	size_t index;
	size_t unique_count;

	if ((result_modes == NULL) || (result_count == NULL)) {
		free(modes);
		return FB_GFX3_INVALID;
	}
	if (count == 0) {
		free(modes);
		return FB_GFX3_UNSUPPORTED;
	}
	qsort(modes, count, sizeof(*modes), screenlist_mode_compare);
	unique_count = 1;
	for (index = 1; index < count; ++index) {
		if (modes[index] != modes[unique_count - 1u])
			modes[unique_count++] = modes[index];
	}
	*result_modes = modes;
	*result_count = unique_count;
	return FB_GFX3_OK;
}

/* end of gfx3_screenlist_common.c */
