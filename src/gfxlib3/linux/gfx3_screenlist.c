/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_screenlist.c

    Purpose:

        Enumerate X11 display modes for the public SCREENLIST API.

    Responsibilities:

        - keep XRandR optional at build time and run time
        - collect modes advertised by the active X11 screen
        - pass native modes through the shared checked collection helpers

    This file intentionally does NOT contain:

        - iterator state or standard-mode fallback behavior
        - display-mode changes, fullscreen policy, or window creation
        - Win32 or Android display enumeration
*/

#include "../gfx3_screenlist_internal.h"

#include <dlfcn.h>
#include <X11/Xlib.h>

/*
    The RandR ABI used here has been stable since RandR 1.0. Repeating the
    small opaque configuration and size declarations lets the library load
    RandR only where it is present, without requiring Xrandr development
    headers or adding a mandatory final-program link dependency.
*/
typedef struct FB_GFX3_XRR_CONFIGURATION FB_GFX3_XRR_CONFIGURATION;

typedef struct FB_GFX3_XRR_SCREEN_SIZE {
	int width;
	int height;
	int mwidth;
	int mheight;
} FB_GFX3_XRR_SCREEN_SIZE;

typedef FB_GFX3_XRR_CONFIGURATION *(*FB_GFX3_XRR_GET_SCREEN_INFO)(
	Display *display, Drawable drawable);
typedef FB_GFX3_XRR_SCREEN_SIZE *(*FB_GFX3_XRR_CONFIG_SIZES)(
	FB_GFX3_XRR_CONFIGURATION *configuration, int *count);
typedef void (*FB_GFX3_XRR_FREE_SCREEN_INFO)(
	FB_GFX3_XRR_CONFIGURATION *configuration);

/* See the matching GLX loader for why this copies dlsym() results. */
static int screenlist_x11_load_function(void *library, const char *name,
	void *destination, size_t destination_size)
{
	void *symbol;

	if ((library == NULL) || (name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(symbol)))
		return FB_GFX3_INVALID;
	symbol = dlsym(library, name);
	if (symbol == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &symbol, sizeof(symbol));
	return FB_GFX3_OK;
}

int fb_gfx3_platform_screenlist_modes(int depth, int **modes,
	size_t *mode_count)
{
	FB_GFX3_XRR_GET_SCREEN_INFO get_screen_info = NULL;
	FB_GFX3_XRR_CONFIG_SIZES config_sizes = NULL;
	FB_GFX3_XRR_FREE_SCREEN_INFO free_screen_info = NULL;
	FB_GFX3_XRR_CONFIGURATION *configuration = NULL;
	FB_GFX3_XRR_SCREEN_SIZE *sizes;
	Display *display = NULL;
	void *library = NULL;
	int *result = NULL;
	int size_count = 0;
	int index;
	size_t count = 0;
	size_t capacity = 0;
	int status = FB_GFX3_UNSUPPORTED;

	if ((modes == NULL) || (mode_count == NULL) || (depth <= 0))
		return FB_GFX3_INVALID;
	*modes = NULL;
	*mode_count = 0;
	if (!fb_gfx3_screenlist_depth_matches(8u, depth) &&
	    !fb_gfx3_screenlist_depth_matches(16u, depth) &&
	    !fb_gfx3_screenlist_depth_matches(32u, depth))
		return FB_GFX3_UNSUPPORTED;

	display = XOpenDisplay(NULL);
	if (display == NULL)
		return FB_GFX3_UNSUPPORTED;
	library = dlopen("libXrandr.so.2", RTLD_LAZY | RTLD_LOCAL);
	if (library == NULL)
		library = dlopen("libXrandr.so", RTLD_LAZY | RTLD_LOCAL);
	if (library == NULL)
		goto cleanup;
	if (screenlist_x11_load_function(library, "XRRGetScreenInfo",
	    &get_screen_info, sizeof(get_screen_info)) != FB_GFX3_OK)
		goto cleanup;
	if (screenlist_x11_load_function(library, "XRRConfigSizes",
	    &config_sizes, sizeof(config_sizes)) != FB_GFX3_OK)
		goto cleanup;
	if (screenlist_x11_load_function(library, "XRRFreeScreenConfigInfo",
	    &free_screen_info, sizeof(free_screen_info)) != FB_GFX3_OK)
		goto cleanup;
	if ((get_screen_info == NULL) || (config_sizes == NULL) ||
	    (free_screen_info == NULL))
		goto cleanup;

	configuration = get_screen_info(display,
		RootWindow(display, DefaultScreen(display)));
	if (configuration == NULL)
		goto cleanup;
	sizes = config_sizes(configuration, &size_count);
	if ((sizes == NULL) || (size_count <= 0))
		goto cleanup;
	for (index = 0; index < size_count; ++index) {
		if ((sizes[index].width <= 0) || (sizes[index].height <= 0) ||
		    (sizes[index].width > 0x7FFF) || (sizes[index].height > 0xFFFF))
			continue;
		if (fb_gfx3_screenlist_append(&result, &count, &capacity,
			(sizes[index].width << 16) | sizes[index].height) !=
		    FB_GFX3_OK) {
			status = FB_GFX3_OUT_OF_MEMORY;
			goto cleanup;
		}
	}
	status = fb_gfx3_screenlist_finish(result, count, modes, mode_count);
	result = NULL;

cleanup:
	free(result);
	if (configuration != NULL)
		free_screen_info(configuration);
	/*
		RandR registers display-owned extension callbacks when its functions
		first touch this Display. XCloseDisplay() calls those callbacks, so
		libXrandr must remain loaded until after the Display is closed.
	*/
	XCloseDisplay(display);
	if (library != NULL)
		dlclose(library);
	return status;
}

/* end of linux/gfx3_screenlist.c */
