/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_vulkan_platform.c

    Purpose:

        Connect the shared Vulkan runtime to the Linux loader and Xlib surface
        ABI.

    Responsibilities:

        - load libvulkan and resolve exported functions
        - validate Xlib display and window handles
        - initialize VkXlibSurfaceCreateInfoKHR-compatible storage

    This file intentionally does NOT contain:

        - Vulkan device, queue, or resource management
        - X11 window creation
        - GLX or OpenGL integration
*/

#include "../gfx3_vulkan_platform.h"

#include <dlfcn.h>

#define FB_GFX3_VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO 1000004000u

typedef struct FB_GFX3_VK_XLIB_SURFACE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t flags;
	void *display;
	unsigned long window;
} FB_GFX3_VK_XLIB_SURFACE_CREATE_INFO;

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void)
{
	return dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
}

void fb_gfx3_vulkan_platform_library_close(
	FB_GFX3_VULKAN_LIBRARY library)
{
	if (library != NULL)
		dlclose(library);
}

int fb_gfx3_vulkan_platform_load_library_function(
	FB_GFX3_VULKAN_LIBRARY library, const char *name, void *destination,
	size_t destination_size)
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

const char *fb_gfx3_vulkan_platform_instance_extension(void)
{
	return "VK_KHR_xlib_surface";
}

const char *fb_gfx3_vulkan_platform_create_surface_function(void)
{
	return "vkCreateXlibSurfaceKHR";
}

int fb_gfx3_vulkan_platform_window_valid(uintptr_t native_instance,
	uintptr_t native_window, uint32_t width, uint32_t height)
{
	return (native_instance != 0) && (native_window != 0) &&
		(width != 0u) && (height != 0u);
}

int fb_gfx3_vulkan_platform_surface_create_info(
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO *create_info,
	uintptr_t native_instance, uintptr_t native_window)
{
	FB_GFX3_VK_XLIB_SURFACE_CREATE_INFO *xlib_info;

	if ((create_info == NULL) ||
	    (sizeof(*xlib_info) > sizeof(*create_info)) ||
	    (native_instance == 0) || (native_window == 0))
		return FB_GFX3_INVALID;
	memset(create_info, 0, sizeof(*create_info));
	xlib_info = (FB_GFX3_VK_XLIB_SURFACE_CREATE_INFO *)create_info;
	xlib_info->structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO;
	xlib_info->display = (void *)native_instance;
	xlib_info->window = (unsigned long)native_window;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_platform_resolve_instance_version(void)
{
	return TRUE;
}

/* end of linux/gfx3_vulkan_platform.c */
