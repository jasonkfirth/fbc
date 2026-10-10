'' Project: FreeBASIC SDL3 bindings
'' File: SDL_vulkan.bi
'' Purpose: Declare the SDL3-3.4.18 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Simple DirectMedia Layer
''   Copyright (C) 2017, Mark Callow
''
''   This software is provided 'as-is', without any express or implied
''   warranty.  In no event will the authors be held liable for any damages
''   arising from the use of this software.
''
''   Permission is granted to anyone to use this software for any purpose,
''   including commercial applications, and to alter it and redistribute it
''   freely, subject to the following restrictions:
''
''   1. The origin of this software must not be misrepresented; you must not
''      claim that you wrote the original software. If you use this software
''      in a product, an acknowledgment in the product documentation would be
''      appreciated but is not required.
''   2. Altered source versions must be plainly marked as such, and must not be
''      misrepresented as being the original software.
''   3. This notice may not be removed or altered from any source distribution.

#pragma once

#include once "SDL.bi"

extern "C"

'' -------------------------------------------------------------------------
'' SDL_vulkan.h
'' -------------------------------------------------------------------------
#ifdef __FB_64BIT__
#else
#endif

#ifndef VK_VERSION_1_0
type VkInstance as VkInstance_T ptr
type VkPhysicalDevice as VkPhysicalDevice_T ptr

#ifdef __FB_64BIT__
	type VkSurfaceKHR as VkSurfaceKHR_T ptr
#else
	type VkSurfaceKHR as ulongint
#endif

type VkAllocationCallbacks as VkAllocationCallbacks_
#endif

declare function SDL_Vulkan_LoadLibrary(byval path as const zstring ptr) as boolean
declare function SDL_Vulkan_GetVkGetInstanceProcAddr() as SDL_FunctionPointer
declare sub SDL_Vulkan_UnloadLibrary()
declare function SDL_Vulkan_GetInstanceExtensions(byval count as Uint32 ptr) as const zstring const ptr ptr
declare function SDL_Vulkan_CreateSurface(byval window as SDL_Window ptr, byval instance as VkInstance, byval allocator as const VkAllocationCallbacks ptr, byval surface as VkSurfaceKHR ptr) as boolean
declare sub SDL_Vulkan_DestroySurface(byval instance as VkInstance, byval surface as VkSurfaceKHR, byval allocator as const VkAllocationCallbacks ptr)
declare function SDL_Vulkan_GetPresentationSupport(byval instance as VkInstance, byval physicalDevice as VkPhysicalDevice, byval queueFamilyIndex as Uint32) as boolean

end extern

'' end of SDL_vulkan.bi
