/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: win32/gfx3_joystick.c

    Purpose:

        Poll the legacy Win32 joystick API used by gfxlib2 GETJOYSTICK.

    Responsibilities:

        - load WinMM only when a program actually requests a joystick
        - normalize the eight legacy axes and point-of-view hat safely
        - retain gfxlib2's missing-device values and slot numbering

    This file intentionally does NOT contain:

        - XInput GETXPAD support
        - controller event dispatch or render-thread state
        - graphics window management
*/

#include "../gfx3_joystick.h"

#if defined(HOST_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

typedef MMRESULT (WINAPI *FB_GFX3_JOY_GET_DEV_CAPS)(UINT_PTR id,
	LPJOYCAPSA capabilities, UINT capability_size);
typedef MMRESULT (WINAPI *FB_GFX3_JOY_GET_POS_EX)(UINT_PTR id,
	LPJOYINFOEX information);

typedef struct FB_GFX3_WIN32_JOYSTICK_API {
	HMODULE library;
	FB_GFX3_JOY_GET_DEV_CAPS get_device_capabilities;
	FB_GFX3_JOY_GET_POS_EX get_position;
} FB_GFX3_WIN32_JOYSTICK_API;

typedef struct FB_GFX3_WIN32_JOYSTICK_CAPS_CACHE {
	JOYCAPSA capabilities;
	ULONGLONG checked_milliseconds;
	int known;
	int available;
} FB_GFX3_WIN32_JOYSTICK_CAPS_CACHE;

static INIT_ONCE joystick_api_once = INIT_ONCE_STATIC_INIT;
static FB_GFX3_WIN32_JOYSTICK_API joystick_api;
static SRWLOCK joystick_caps_lock = SRWLOCK_INIT;
static FB_GFX3_WIN32_JOYSTICK_CAPS_CACHE joystick_caps_cache[16];

/* A missing legacy device is rechecked often enough for practical hotplug. */
#define FB_GFX3_JOYSTICK_MISSING_CACHE_MILLISECONDS 1000u

/* ------------------------------------------------------------------------- */
/* One-time optional WinMM loading                                           */
/* ------------------------------------------------------------------------- */

static BOOL CALLBACK joystick_load_api(PINIT_ONCE once, PVOID parameter,
	PVOID *context)
{
	FARPROC procedure;

	(void)once;
	(void)parameter;
	(void)context;
	joystick_api.library = LoadLibraryA("winmm.dll");
	if (joystick_api.library == NULL)
		return TRUE;
	procedure = GetProcAddress(joystick_api.library, "joyGetDevCapsA");
	if ((procedure == NULL) ||
	    (sizeof(joystick_api.get_device_capabilities) != sizeof(procedure)))
		goto unavailable;
	memcpy((void *)&joystick_api.get_device_capabilities,
		(const void *)&procedure,
		sizeof(joystick_api.get_device_capabilities));
	procedure = GetProcAddress(joystick_api.library, "joyGetPosEx");
	if ((procedure == NULL) ||
	    (sizeof(joystick_api.get_position) != sizeof(procedure)))
		goto unavailable;
	memcpy((void *)&joystick_api.get_position, (const void *)&procedure,
		sizeof(joystick_api.get_position));
	return TRUE;

unavailable:
	FreeLibrary(joystick_api.library);
	memset(&joystick_api, 0, sizeof(joystick_api));
	return TRUE;
}

static void joystick_clear_outputs(ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8)
{
	float *axes[] = { axis1, axis2, axis3, axis4, axis5, axis6, axis7,
		axis8 };
	size_t i;

	if (buttons != NULL)
		*buttons = -1;
	for (i = 0; i < sizeof(axes) / sizeof(axes[0]); ++i) {
		if (axes[i] != NULL)
			*axes[i] = -1000.0f;
	}
}

static float joystick_normalize_axis(DWORD value, UINT minimum, UINT maximum)
{
	uint64_t range;
	double normalized;

	if (maximum <= minimum)
		return 0.0f;
	if (value < minimum)
		value = minimum;
	if (value > maximum)
		value = maximum;
	range = (uint64_t)maximum - minimum;
	normalized = (((double)((uint64_t)value - minimum) * 2.0) /
		(double)range) - 1.0;
	return (float)normalized;
}

static int joystick_get_capabilities(UINT_PTR native_id,
	JOYCAPSA *capabilities)
{
	FB_GFX3_WIN32_JOYSTICK_CAPS_CACHE *cache;
	ULONGLONG now;
	MMRESULT query_result;
	int cached_available;
	int use_cache;

	if ((native_id >= 16u) || (capabilities == NULL))
		return FALSE;
	now = GetTickCount64();
	cache = &joystick_caps_cache[native_id];
	AcquireSRWLockShared(&joystick_caps_lock);
	use_cache = cache->known && (cache->available ||
		((now - cache->checked_milliseconds) <
		 FB_GFX3_JOYSTICK_MISSING_CACHE_MILLISECONDS));
	cached_available = cache->available;
	if (use_cache && cached_available)
		*capabilities = cache->capabilities;
	ReleaseSRWLockShared(&joystick_caps_lock);
	if (use_cache)
		return cached_available;

	memset(capabilities, 0, sizeof(*capabilities));
	query_result = joystick_api.get_device_capabilities(native_id,
		capabilities, sizeof(*capabilities));
	AcquireSRWLockExclusive(&joystick_caps_lock);
	cache->checked_milliseconds = now;
	cache->known = TRUE;
	cache->available = (query_result == JOYERR_NOERROR);
	if (cache->available)
		cache->capabilities = *capabilities;
	ReleaseSRWLockExclusive(&joystick_caps_lock);
	return query_result == JOYERR_NOERROR;
}

/* ------------------------------------------------------------------------- */
/* gfxlib2-compatible WinMM query                                           */
/* ------------------------------------------------------------------------- */

int fb_gfx3_platform_joystick_has_native_polling(void)
{
	return TRUE;
}

int fb_gfx3_platform_joystick_get(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8)
{
	JOYCAPSA capabilities;
	JOYINFOEX information;
	UINT_PTR native_id;

	joystick_clear_outputs(buttons, axis1, axis2, axis3, axis4, axis5,
		axis6, axis7, axis8);
	if ((id < 0) || (id >= 16))
		return FB_GFX3_INVALID;
	if (!InitOnceExecuteOnce(&joystick_api_once, joystick_load_api, NULL,
		NULL) || (joystick_api.get_device_capabilities == NULL) ||
	    (joystick_api.get_position == NULL))
		return FB_GFX3_UNSUPPORTED;
	native_id = (UINT_PTR)id + JOYSTICKID1;
	if (!joystick_get_capabilities(native_id, &capabilities))
		return FB_GFX3_UNSUPPORTED;
	memset(&information, 0, sizeof(information));
	information.dwSize = sizeof(information);
	information.dwFlags = JOY_RETURNALL;
	if (joystick_api.get_position(native_id, &information) != JOYERR_NOERROR)
		return FB_GFX3_UNSUPPORTED;
	if (buttons != NULL)
		*buttons = (ssize_t)information.dwButtons;
	if (axis1 != NULL)
		*axis1 = joystick_normalize_axis(information.dwXpos,
			capabilities.wXmin, capabilities.wXmax);
	if (axis2 != NULL)
		*axis2 = joystick_normalize_axis(information.dwYpos,
			capabilities.wYmin, capabilities.wYmax);
	if ((axis3 != NULL) && (capabilities.wCaps & JOYCAPS_HASZ))
		*axis3 = joystick_normalize_axis(information.dwZpos,
			capabilities.wZmin, capabilities.wZmax);
	if ((axis4 != NULL) && (capabilities.wCaps & JOYCAPS_HASR))
		*axis4 = joystick_normalize_axis(information.dwRpos,
			capabilities.wRmin, capabilities.wRmax);
	if ((axis5 != NULL) && (capabilities.wCaps & JOYCAPS_HASU))
		*axis5 = joystick_normalize_axis(information.dwUpos,
			capabilities.wUmin, capabilities.wUmax);
	if ((axis6 != NULL) && (capabilities.wCaps & JOYCAPS_HASV))
		*axis6 = joystick_normalize_axis(information.dwVpos,
			capabilities.wVmin, capabilities.wVmax);
	if ((capabilities.wCaps & JOYCAPS_HASPOV) &&
	    (information.dwPOV != JOY_POVCENTERED)) {
		if (axis7 != NULL) {
			if ((information.dwPOV > 2250u) &&
			    (information.dwPOV < 15750u))
				*axis7 = 1.0f;
			else if ((information.dwPOV > 20250u) &&
			    (information.dwPOV < 33750u))
				*axis7 = -1.0f;
			else
				*axis7 = 0.0f;
		}
		if (axis8 != NULL) {
			if ((information.dwPOV > 11250u) &&
			    (information.dwPOV < 24750u))
				*axis8 = 1.0f;
			else if ((information.dwPOV < 6750u) ||
			    ((information.dwPOV > 29250u) &&
			     (information.dwPOV < 36000u)))
				*axis8 = -1.0f;
			else
				*axis8 = 0.0f;
		}
	}
	return FB_GFX3_OK;
}

#else

int fb_gfx3_platform_joystick_has_native_polling(void)
{
	return FALSE;
}

int fb_gfx3_platform_joystick_get(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8)
{
	float *axes[] = { axis1, axis2, axis3, axis4, axis5, axis6, axis7,
		axis8 };
	size_t i;

	(void)id;
	if (buttons != NULL)
		*buttons = -1;
	for (i = 0; i < sizeof(axes) / sizeof(axes[0]); ++i) {
		if (axes[i] != NULL)
			*axes[i] = -1000.0f;
	}
	return FB_GFX3_UNSUPPORTED;
}

#endif

/* end of win32/gfx3_joystick.c */
