/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_erasestr.c
 * Purpose: ERASE for dynamic arrays of var-len strings.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_ArrayStrErase( FBARRAY *array )
{
	fb_ArrayDestructStr( array );

	/* only free the memory if it's not a fixed length array */
	if( array && !(array->flags & FBARRAY_FLAGS_FIXED_LEN) ) {
		fb_ArrayErase( array );
	}
}

/* end of arrays/array_erasestr.c */
