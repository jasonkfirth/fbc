/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_settime.c
 * Purpose: Provide time settime runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hSetTime( int h, int m, int s )
{
	/* TODO: use NtSetSystemTime */
	return 0;
}

/* end of drivers/xbox/time_settime.c */
