'' Project: FreeBASIC SDL3 bindings
'' File: SDL_platform_api.bi
'' Purpose: Declare the mobile and GDK APIs outside the desktop header profiles.
'' Responsibilities: Preserve C callbacks and opaque platform handles.
'' This file intentionally does NOT contain: native startup code or platform emulation.
''
'' Translated from SDL3 3.4.18's SDL_system.h; this is an altered source version.
'' The complete upstream zlib notice is preserved in SDL.bi.

#pragma once

extern "C"

#if defined(__FB_ANDROID__) or defined(SDL_PLATFORM_ANDROID)
	'' JNIEnv belongs to the current native thread. SDL deliberately exposes it
	'' as void* so applications choose their own JNI declarations.
	declare function SDL_GetAndroidJNIEnv() as any ptr
	declare function SDL_GetAndroidActivity() as any ptr
	declare function SDL_GetAndroidSDKVersion() as long
	declare function SDL_IsChromebook() as boolean
	declare function SDL_IsDeXMode() as boolean
	declare sub SDL_SendAndroidBackButton()
	const SDL_ANDROID_EXTERNAL_STORAGE_READ = &h01
	const SDL_ANDROID_EXTERNAL_STORAGE_WRITE = &h02
	declare function SDL_GetAndroidInternalStoragePath() as const zstring ptr
	declare function SDL_GetAndroidExternalStorageState() as Uint32
	declare function SDL_GetAndroidExternalStoragePath() as const zstring ptr
	declare function SDL_GetAndroidCachePath() as const zstring ptr
	'' A permission result may arrive on another thread or during submission.
	'' Keep userdata alive until the callback has finished.
	type SDL_RequestAndroidPermissionCallback as sub(byval userdata as any ptr, byval permission as const zstring ptr, byval granted as boolean)
	declare function SDL_RequestAndroidPermission(byval permission as const zstring ptr, byval cb as SDL_RequestAndroidPermissionCallback, byval userdata as any ptr) as boolean
	declare function SDL_ShowAndroidToast(byval message as const zstring ptr, byval duration as long, byval gravity as long, byval xoffset as long, byval yoffset as long) as boolean
	declare function SDL_SendAndroidMessage(byval command as Uint32, byval param as long) as boolean
#endif

#ifdef SDL_PLATFORM_IOS
	type SDL_iOSAnimationCallback as sub(byval userdata as any ptr)
	declare function SDL_SetiOSAnimationCallback(byval window as SDL_Window ptr, byval interval as long, byval callback as SDL_iOSAnimationCallback, byval callbackParam as any ptr) as boolean
	declare sub SDL_SetiOSEventPump(byval enabled as boolean)
	declare sub SDL_OnApplicationDidChangeStatusBarOrientation()
#endif

#ifdef SDL_PLATFORM_GDK
	type XTaskQueueHandle as XTaskQueueObject ptr
	type XUserHandle as XUser ptr
	'' These handles are reference-counted by GDK. Close a task-queue reference
	'' with XTaskQueueCloseHandle once it is no longer needed.
	declare function SDL_GetGDKTaskQueue(byval outTaskQueue as XTaskQueueHandle ptr) as boolean
	declare function SDL_GetGDKDefaultUser(byval outUserHandle as XUserHandle ptr) as boolean
#endif

end extern

'' end of SDL_platform_api.bi
