/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vulkan_platform.h

    Purpose:

        Define the private operating-system boundary used by the shared Vulkan
        runtime.

    Responsibilities:

        - load and close the platform Vulkan loader
        - resolve exported loader functions without unsafe pointer casts
        - describe and initialize the native Vulkan surface boundary
        - document platform-specific loader capability policy

    This file intentionally does NOT contain:

        - Vulkan device, queue, or resource management
        - native window creation
        - public graphics API declarations
*/

#ifndef __FB_GFX3_VULKAN_PLATFORM_H__
#define __FB_GFX3_VULKAN_PLATFORM_H__

#include "fb_gfx3.h"

#define FB_GFX3_VULKAN_SURFACE_CREATE_INFO_SIZE 64u

typedef void *FB_GFX3_VULKAN_LIBRARY;

typedef union FB_GFX3_VULKAN_SURFACE_CREATE_INFO {
	max_align_t alignment;
	unsigned char bytes[FB_GFX3_VULKAN_SURFACE_CREATE_INFO_SIZE];
} FB_GFX3_VULKAN_SURFACE_CREATE_INFO;

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void);
void fb_gfx3_vulkan_platform_library_close(
	FB_GFX3_VULKAN_LIBRARY library);
int fb_gfx3_vulkan_platform_load_library_function(
	FB_GFX3_VULKAN_LIBRARY library, const char *name, void *destination,
	size_t destination_size);
const char *fb_gfx3_vulkan_platform_instance_extension(void);
const char *fb_gfx3_vulkan_platform_create_surface_function(void);
int fb_gfx3_vulkan_platform_window_valid(uintptr_t native_instance,
	uintptr_t native_window, uint32_t width, uint32_t height);
int fb_gfx3_vulkan_platform_surface_create_info(
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO *create_info,
	uintptr_t native_instance, uintptr_t native_window);
int fb_gfx3_vulkan_platform_resolve_instance_version(void);

#endif

/* end of gfx3_vulkan_platform.h */
