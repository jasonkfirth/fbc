'' Project: FreeBASIC SDL3 binding tests
'' File: platform-declarations.bas
'' Purpose: Check signatures omitted by the desktop C header profiles.
'' Responsibilities: Compile Android, iOS, and GDK calls and callback types.
'' This file intentionally does NOT contain: executable platform tests.

'' These feature guards expose declarations for emission checks. The calls
'' below are never linked or run on a machine without the corresponding SDK.
#define SDL_PLATFORM_ANDROID
#define SDL_PLATFORM_IOS
#define SDL_PLATFORM_GDK
#include once "SDL3/SDL.bi"

private sub permission_result cdecl(byval userdata as any ptr, byval permission as const zstring ptr, byval granted as boolean)
end sub

private sub animation_frame cdecl(byval userdata as any ptr)
end sub

sub check_platform_declarations()
	var environment = SDL_GetAndroidJNIEnv()
	var activity = SDL_GetAndroidActivity()
	var version = SDL_GetAndroidSDKVersion()
	var chromebook = SDL_IsChromebook()
	var dexmode = SDL_IsDeXMode()
	SDL_SendAndroidBackButton()
	var internal_path = SDL_GetAndroidInternalStoragePath()
	var external_state = SDL_GetAndroidExternalStorageState()
	var external_path = SDL_GetAndroidExternalStoragePath()
	var cache_path = SDL_GetAndroidCachePath()
	var requested = SDL_RequestAndroidPermission(strptr("android.permission.CAMERA"), @permission_result, 0)
	var shown = SDL_ShowAndroidToast(strptr("SDL3"), 0, -1, 0, 0)
	var sent = SDL_SendAndroidMessage(&h8000, 0)

	var animated = SDL_SetiOSAnimationCallback(0, 1, @animation_frame, 0)
	SDL_SetiOSEventPump(true)
	SDL_OnApplicationDidChangeStatusBarOrientation()

	dim task_queue as XTaskQueueHandle
	dim user_handle as XUserHandle
	var queue_available = SDL_GetGDKTaskQueue(@task_queue)
	var user_available = SDL_GetGDKDefaultUser(@user_handle)
end sub

'' end of platform-declarations.bas
