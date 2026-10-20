/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_getdesc.c
 * Purpose: fbc-int API: array descriptor internals.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBARRAY *fb_ArrayGetDesc( FBARRAY *array )
{
	return array;
}

/* end of arrays/array_getdesc.c */
