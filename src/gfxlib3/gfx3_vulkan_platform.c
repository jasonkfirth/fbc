/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vulkan_platform.c

    Purpose:

        Provide the Vulkan loader boundary on targets without native Vulkan
        integration.

    Responsibilities:

        - reject loader and surface requests predictably
        - keep the shared Vulkan runtime linkable on unsupported targets

    This file intentionally does NOT contain:

        - dynamic-library system calls
        - native surface declarations
        - Vulkan device or resource management
*/

#include "gfx3_vulkan_platform.h"

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void)
{
	return NULL;
}

void fb_gfx3_vulkan_platform_library_close(
	FB_GFX3_VULKAN_LIBRARY library)
{
	(void)library;
}

int fb_gfx3_vulkan_platform_load_library_function(
	FB_GFX3_VULKAN_LIBRARY library, const char *name, void *destination,
	size_t destination_size)
{
	(void)library;
	(void)name;
	(void)destination;
	(void)destination_size;
	return FB_GFX3_UNSUPPORTED;
}

const char *fb_gfx3_vulkan_platform_instance_extension(void)
{
	return NULL;
}

const char *fb_gfx3_vulkan_platform_create_surface_function(void)
{
	return NULL;
}

int fb_gfx3_vulkan_platform_window_valid(uintptr_t native_instance,
	uintptr_t native_window, uint32_t width, uint32_t height)
{
	(void)native_instance;
	(void)native_window;
	(void)width;
	(void)height;
	return FALSE;
}

int fb_gfx3_vulkan_platform_surface_create_info(
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO *create_info,
	uintptr_t native_instance, uintptr_t native_window)
{
	(void)create_info;
	(void)native_instance;
	(void)native_window;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_platform_resolve_instance_version(void)
{
	return FALSE;
}

/* end of gfx3_vulkan_platform.c */
