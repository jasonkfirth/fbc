/*
 * Project: FreeBASIC Windows runtime: sys_hidle.c
 * Purpose: Supply monotonic cooperative-idle deadlines.
 * Responsibilities: Read the system performance counter without shared state.
 * This file contains no worker thread, timer window or scheduling policy.
 * File: drivers/win32/sys_hidle.c
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#include "../../fb_services.h"
#include <windows.h>
int fb_hIdleNow(double *seconds)
{
	LARGE_INTEGER counter, frequency;
	if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
	    !QueryPerformanceCounter(&counter))
		return FALSE;
	*seconds = (double)counter.QuadPart / (double)frequency.QuadPart;
	return TRUE;
}

/* end of drivers/win32/sys_hidle.c */
