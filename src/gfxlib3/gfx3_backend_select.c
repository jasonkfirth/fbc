/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_select.c

    Purpose:

        Choose the best usable gfxlib3 renderer in the same ordered-fallback
        style used by gfxlib2's platform driver lists.

    Responsibilities:

        - rank the GPU backends compiled for the current platform
        - prefer a requested FBGFX or SCREENCONTROL driver before fallback
        - preserve explicit null and Vulkan SCREEN flag behavior
        - store a bounded, process-wide driver-name override

    This file intentionally does NOT contain:

        - renderer probing, initialization, or shutdown
        - windowed/fullscreen retry policy
        - FreeBASIC SCREEN or SCREENCONTROL entry points
*/

#include "gfx3_backend_select.h"

#include "gfx3_backend_gles.h"
#include "gfx3_backend_null.h"
#include "gfx3_backend_opengl.h"
#include "gfx3_backend_vulkan.h"
#include "gfx3_target.h"

#include <ctype.h>

#define FB_GFX3_SCREEN_OPENGL 0x00000002u
#define FB_GFX3_SCREEN_FULLSCREEN 0x00000001u
#define FB_GFX3_SCREEN_VULKAN 0x00000200u
#define FB_GFX3_SCREEN_RESIZABLE 0x00000400u
#define FB_GFX3_DRIVER_NAME_LIMIT 127u

static char *requested_driver_name;

/* ------------------------------------------------------------------------- */
/* Driver-name handling                                                      */
/* ------------------------------------------------------------------------- */

static int backend_name_equal(const char *left, const char *right)
{
	unsigned char left_character;
	unsigned char right_character;

	if ((left == NULL) || (right == NULL))
		return FALSE;
	do {
		left_character = (unsigned char)*left++;
		right_character = (unsigned char)*right++;
		if (tolower(left_character) != tolower(right_character))
			return FALSE;
	} while ((left_character != '\0') && (right_character != '\0'));
	return left_character == right_character;
}

static int backend_matches_name(const FB_GFX3_BACKEND_VTABLE *backend,
	const char *name)
{
	if ((backend == NULL) || (name == NULL))
		return FALSE;
	if (backend_name_equal(backend->name, name))
		return TRUE;
	if (backend == &__fb_gfx3_backend_vulkan)
		return backend_name_equal(name, "vulkan") ||
			backend_name_equal(name, "vk");
	if (backend == &__fb_gfx3_backend_opengl)
		return backend_name_equal(name, "opengl") ||
			backend_name_equal(name, "gl");
	if (backend == &__fb_gfx3_backend_gles)
		return backend_name_equal(name, "opengl es") ||
			backend_name_equal(name, "opengles") ||
			backend_name_equal(name, "gles");
	return backend == &__fb_gfx3_backend_null &&
		backend_name_equal(name, "null");
}

const char *fb_gfx3_backend_requested_name(void)
{
	return requested_driver_name;
}

int fb_gfx3_backend_set_requested_name(const char *name, size_t length)
{
	char *replacement = NULL;

	if ((name == NULL) && (length != 0))
		return FB_GFX3_INVALID;
	if (length > FB_GFX3_DRIVER_NAME_LIMIT)
		length = FB_GFX3_DRIVER_NAME_LIMIT;
	if (length != 0) {
		replacement = (char *)malloc(length + 1u);
		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		memcpy(replacement, name, length);
		replacement[length] = '\0';
	}
	free(requested_driver_name);
	requested_driver_name = replacement;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Platform ranking                                                         */
/* ------------------------------------------------------------------------- */

size_t fb_gfx3_backend_plan(int flags, const char *requested_name,
	const FB_GFX3_BACKEND_VTABLE **plan, size_t capacity)
{
	const FB_GFX3_BACKEND_VTABLE *backends[3];
	const size_t backend_capacity = sizeof(backends) / sizeof(backends[0]);
	size_t backend_count;
	size_t plan_count = 0;
	size_t index;

	if ((plan == NULL) || (capacity == 0))
		return 0;
	if ((flags == -1) || backend_name_equal(requested_name, "null")) {
		plan[0] = &__fb_gfx3_backend_null;
		return 1;
	}
	if (((uint32_t)flags & FB_GFX3_SCREEN_VULKAN) != 0) {
		plan[0] = &__fb_gfx3_backend_vulkan;
		return 1;
	}
	if (((uint32_t)flags & FB_GFX3_SCREEN_OPENGL) != 0) {
		plan[0] = fb_gfx3_target_opengl_backend();
		if (plan[0] == NULL)
			return 0;
		return 1;
	}

	backend_count = fb_gfx3_target_backend_default_list(backends,
		backend_capacity);
	if (backend_count > backend_capacity)
		return 0;
	if ((requested_name != NULL) && (requested_name[0] != '\0')) {
		for (index = 0; index < backend_count; index++) {
			if (backend_matches_name(backends[index], requested_name)) {
				plan[plan_count++] = backends[index];
				break;
			}
		}
	}

	/*
	    gfxlib2 falls back through its complete platform list after a named
	    driver cannot initialize.  Do the same here, while avoiding a second
	    identical attempt because gfxlib3 currently has no refresh-rate variant
	    that would make that retry materially different.
	*/
	for (index = 0; (index < backend_count) && (plan_count < capacity); index++) {
		if ((plan_count != 0) && (plan[0] == backends[index]))
			continue;
		plan[plan_count++] = backends[index];
	}
	return plan_count;
}

/* ------------------------------------------------------------------------- */
/* Window-mode retry plan                                                    */
/* ------------------------------------------------------------------------- */

size_t fb_gfx3_backend_attempt_plan(int flags, const char *requested_name,
	const FB_GFX3_BACKEND_VTABLE **plan, int *attempt_flags,
	size_t capacity)
{
	const FB_GFX3_BACKEND_VTABLE *backends[3];
	const FB_GFX3_BACKEND_VTABLE *requested_backend = NULL;
	const size_t backend_capacity = sizeof(backends) / sizeof(backends[0]);
	size_t backend_count;
	size_t attempt_count = 0;
	size_t pass;
	size_t index;

	if ((plan == NULL) || (attempt_flags == NULL) || (capacity == 0))
		return 0;
	/*
		Explicit renderer flags are gfxlib3 extensions.  They deliberately
		prevent fallback to another GPU API, but retain the historical
		fullscreen inversion retry for a backend which accepts only one native
		surface style on a particular machine.
	*/
	if ((flags == -1) || backend_name_equal(requested_name, "null") ||
	    (((uint32_t)flags & (FB_GFX3_SCREEN_VULKAN |
		FB_GFX3_SCREEN_OPENGL)) != 0)) {
		backend_count = fb_gfx3_backend_plan(flags, requested_name, backends,
			backend_capacity);
		if (backend_count > backend_capacity)
			return 0;
		if (backend_count == 0)
			return 0;
		for (pass = 0; pass <
		    ((((uint32_t)flags & FB_GFX3_SCREEN_RESIZABLE) != 0u) ? 1u : 2u);
		    pass++) {
			for (index = 0; index < backend_count; index++) {
				if (attempt_count >= capacity)
					return attempt_count;
				plan[attempt_count] = backends[index];
				attempt_flags[attempt_count] = (pass == 0) ? flags :
					(flags ^ (int)FB_GFX3_SCREEN_FULLSCREEN);
				attempt_count++;
			}
			if ((backend_count == 1) &&
			    (backends[0] == &__fb_gfx3_backend_null))
				break;
		}
		return attempt_count;
	}

	backend_count = fb_gfx3_target_backend_default_list(backends,
		backend_capacity);
	if ((backend_count == 0) || (backend_count > backend_capacity))
		return 0;
	if ((requested_name != NULL) && (requested_name[0] != '\0')) {
		for (index = 0; index < backend_count; index++) {
			if (backend_matches_name(backends[index], requested_name)) {
				requested_backend = backends[index];
				break;
			}
		}
	}

	/*
		This deliberately matches gfxlib2's gfx_screen.c loop.  A recognized
		FBGFX or SET_DRIVER_NAME value receives a first dedicated attempt.  If
		that fails, the complete platform list is still tried, including the
		requested driver a second time.  gfxlib2 then repeats that exact pair
		of passes with DRIVER_FULLSCREEN inverted.  Retaining the duplicate
		attempt is intentional: legacy platform drivers may distinguish a
		named initialization from their ordinary list retry through state that
		is not visible to the common selector.
	*/
	for (pass = 0; pass <
	    ((((uint32_t)flags & FB_GFX3_SCREEN_RESIZABLE) != 0u) ? 1u : 2u);
	    pass++) {
		int flags_for_pass = (pass == 0) ? flags :
			(flags ^ (int)FB_GFX3_SCREEN_FULLSCREEN);

		if (requested_backend != NULL) {
			if (attempt_count >= capacity)
				return attempt_count;
			plan[attempt_count] = requested_backend;
			attempt_flags[attempt_count] = flags_for_pass;
			attempt_count++;
		}
		for (index = 0; index < backend_count; index++) {
			if (attempt_count >= capacity)
				return attempt_count;
			plan[attempt_count] = backends[index];
			attempt_flags[attempt_count] = flags_for_pass;
			attempt_count++;
		}
	}
	return attempt_count;
}

/* end of gfx3_backend_select.c */
