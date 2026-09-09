/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: darwin/gfx3_vulkan_platform.c

    Purpose:

        Connect the shared Vulkan runtime to MoltenVK and a Cocoa Metal layer.

    Responsibilities:

        - locate a Vulkan or MoltenVK dynamic loader without a link dependency
        - request the Metal surface and portability extensions used by MoltenVK
        - initialize VkMetalSurfaceCreateInfoEXT-compatible storage

    This file intentionally does NOT contain:

        - Cocoa window creation or event translation
        - Vulkan device, queue, or resource management
        - a native Metal renderer
*/

#include "../gfx3_vulkan_platform.h"

#include <dlfcn.h>
#include <stdlib.h>

#define FB_GFX3_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT 0x00000001u
#define FB_GFX3_VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO 1000217000u

typedef struct FB_GFX3_VK_METAL_SURFACE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t flags;
	const void *layer;
} FB_GFX3_VK_METAL_SURFACE_CREATE_INFO;

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void)
{
	static const char *const names[] = {
		"libvulkan.1.dylib",
		"libvulkan.dylib",
		"/usr/local/lib/libvulkan.1.dylib",
		"/opt/homebrew/lib/libvulkan.1.dylib",
		"libMoltenVK.dylib",
		"/usr/local/lib/libMoltenVK.dylib",
		"/opt/homebrew/lib/libMoltenVK.dylib",
		"MoltenVK.framework/MoltenVK",
		"/Library/Frameworks/MoltenVK.framework/MoltenVK",
		"/System/Library/Frameworks/MoltenVK.framework/MoltenVK"
	};
	const char *override_name;
	void *library;
	size_t index;

	/*
		An application may bundle MoltenVK outside the loader's normal search
		path. The override keeps that deployment usable without baking an SDK or
		package-manager location into libfbgfx3.
	*/
	override_name = getenv("FBGFX_VULKAN_LIBRARY");
	if ((override_name != NULL) && (override_name[0] != '\0')) {
		library = dlopen(override_name, RTLD_NOW | RTLD_LOCAL);
		if (library != NULL)
			return library;
	}
	for (index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
		library = dlopen(names[index], RTLD_NOW | RTLD_LOCAL);
		if (library != NULL)
			return library;
	}
	return NULL;
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
	return "VK_EXT_metal_surface";
}

const char *fb_gfx3_vulkan_platform_portability_instance_extension(void)
{
	return "VK_KHR_portability_enumeration";
}

const char *fb_gfx3_vulkan_platform_portability_device_extension(void)
{
	return "VK_KHR_portability_subset";
}

uint32_t fb_gfx3_vulkan_platform_instance_create_flags(void)
{
	return FB_GFX3_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT;
}

const char *fb_gfx3_vulkan_platform_create_surface_function(void)
{
	return "vkCreateMetalSurfaceEXT";
}

int fb_gfx3_vulkan_platform_window_valid(uintptr_t native_instance,
	uintptr_t native_window, uint32_t width, uint32_t height)
{
	return (native_instance != 0u) && (native_window != 0u) &&
		(width != 0u) && (height != 0u);
}

int fb_gfx3_vulkan_platform_surface_create_info(
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO *create_info,
	uintptr_t native_instance, uintptr_t native_window)
{
	FB_GFX3_VK_METAL_SURFACE_CREATE_INFO *metal_info;

	if ((create_info == NULL) ||
	    (sizeof(*metal_info) > sizeof(*create_info)) ||
	    (native_instance == 0u) || (native_window == 0u))
		return FB_GFX3_INVALID;
	memset(create_info, 0, sizeof(*create_info));
	metal_info = (FB_GFX3_VK_METAL_SURFACE_CREATE_INFO *)create_info;
	metal_info->structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO;
	metal_info->layer = (const void *)native_window;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_platform_resolve_instance_version(void)
{
	return TRUE;
}

/* end of darwin/gfx3_vulkan_platform.c */
