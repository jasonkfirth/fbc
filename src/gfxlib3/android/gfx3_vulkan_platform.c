/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: android/gfx3_vulkan_platform.c

    Purpose:

        Connect the shared Vulkan runtime to the Android loader and native
        window surface ABI.

    Responsibilities:

        - load the optional Android Vulkan system library
        - resolve exported functions through dlsym
        - initialize VkAndroidSurfaceCreateInfoKHR-compatible storage

    This file intentionally does NOT contain:

        - Vulkan device, queue, or resource management
        - NativeActivity lifecycle handling
        - EGL or OpenGL ES integration
*/

#include "../gfx3_vulkan_platform.h"

#include <dlfcn.h>

#define FB_GFX3_VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO 1000008000u

typedef struct FB_GFX3_VK_ANDROID_SURFACE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t flags;
	void *window;
} FB_GFX3_VK_ANDROID_SURFACE_CREATE_INFO;

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void)
{
	return dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
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
	return "VK_KHR_android_surface";
}

const char *fb_gfx3_vulkan_platform_create_surface_function(void)
{
	return "vkCreateAndroidSurfaceKHR";
}

int fb_gfx3_vulkan_platform_window_valid(uintptr_t native_instance,
	uintptr_t native_window, uint32_t width, uint32_t height)
{
	(void)native_instance;
	return (native_window != 0) && (width != 0u) && (height != 0u);
}

int fb_gfx3_vulkan_platform_surface_create_info(
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO *create_info,
	uintptr_t native_instance, uintptr_t native_window)
{
	FB_GFX3_VK_ANDROID_SURFACE_CREATE_INFO *android_info;

	(void)native_instance;
	if ((create_info == NULL) ||
	    (sizeof(*android_info) > sizeof(*create_info)) ||
	    (native_window == 0))
		return FB_GFX3_INVALID;
	memset(create_info, 0, sizeof(*create_info));
	android_info = (FB_GFX3_VK_ANDROID_SURFACE_CREATE_INFO *)create_info;
	android_info->structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO;
	android_info->window = (void *)native_window;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_platform_resolve_instance_version(void)
{
	/*
		Android no-HAL stubs can diagnose an optional Vulkan 1.1 query made
		through vkGetInstanceProcAddr. The exported symbol remains safe.
	*/
	return FALSE;
}

/* end of android/gfx3_vulkan_platform.c */
