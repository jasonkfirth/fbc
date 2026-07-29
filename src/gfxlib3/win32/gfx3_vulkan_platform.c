/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: win32/gfx3_vulkan_platform.c

    Purpose:

        Connect the shared Vulkan runtime to the Win32 Vulkan loader and
        surface ABI.

    Responsibilities:

        - load vulkan-1.dll and resolve exported functions
        - validate Win32 instance and window handles
        - initialize VkWin32SurfaceCreateInfoKHR-compatible storage

    This file intentionally does NOT contain:

        - Vulkan device, queue, or resource management
        - Win32 window creation
        - WGL or OpenGL integration
*/

#include "../gfx3_vulkan_platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define FB_GFX3_VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO 1000009000u

typedef struct FB_GFX3_VK_WIN32_SURFACE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t flags;
	HINSTANCE instance;
	HWND window;
} FB_GFX3_VK_WIN32_SURFACE_CREATE_INFO;

FB_GFX3_VULKAN_LIBRARY fb_gfx3_vulkan_platform_library_open(void)
{
	return (FB_GFX3_VULKAN_LIBRARY)LoadLibraryA("vulkan-1.dll");
}

void fb_gfx3_vulkan_platform_library_close(
	FB_GFX3_VULKAN_LIBRARY library)
{
	if (library != NULL)
		FreeLibrary((HMODULE)library);
}

int fb_gfx3_vulkan_platform_load_library_function(
	FB_GFX3_VULKAN_LIBRARY library, const char *name, void *destination,
	size_t destination_size)
{
	FARPROC procedure;

	if ((library == NULL) || (name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(procedure)))
		return FB_GFX3_INVALID;
	procedure = GetProcAddress((HMODULE)library, name);
	if (procedure == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, (const void *)&procedure, sizeof(procedure));
	return FB_GFX3_OK;
}

const char *fb_gfx3_vulkan_platform_instance_extension(void)
{
	return "VK_KHR_win32_surface";
}

const char *fb_gfx3_vulkan_platform_create_surface_function(void)
{
	return "vkCreateWin32SurfaceKHR";
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
	FB_GFX3_VK_WIN32_SURFACE_CREATE_INFO *win32_info;

	if ((create_info == NULL) ||
	    (sizeof(*win32_info) > sizeof(*create_info)) ||
	    (native_instance == 0) || (native_window == 0))
		return FB_GFX3_INVALID;
	memset(create_info, 0, sizeof(*create_info));
	win32_info = (FB_GFX3_VK_WIN32_SURFACE_CREATE_INFO *)create_info;
	win32_info->structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO;
	win32_info->instance = (HINSTANCE)native_instance;
	win32_info->window = (HWND)native_window;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_platform_resolve_instance_version(void)
{
	return TRUE;
}

/* end of win32/gfx3_vulkan_platform.c */
