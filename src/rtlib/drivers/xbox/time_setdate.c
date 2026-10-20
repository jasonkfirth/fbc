/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_setdate.c
 * Purpose: set date function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hSetDate( int y, int m, int d )
{
	/* TODO: use NtSetSystemTime */
	return 0;
}

/* end of drivers/xbox/time_setdate.c */
