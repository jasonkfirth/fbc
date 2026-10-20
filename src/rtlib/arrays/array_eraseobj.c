/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_eraseobj.c
 * Purpose: Provide array eraseobj runtime support.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/* ERASE for dynamic arrays of objects: destruct elements and free the array

   if array is static (fixed length) then perform a clear operation only
*/

#include "fb.h"

FBCALL int fb_ArrayEraseObj( FBARRAY *array, FB_DEFCTOR ctor, FB_DEFCTOR dtor )
{
	if( array->flags & FBARRAY_FLAGS_FIXED_LEN ) {
		fb_ArrayClearObj( array, ctor, dtor );
	} else {
		if( dtor )
			fb_ArrayDestructObj( array, dtor );
		fb_ArrayErase( array );
	}
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of arrays/array_eraseobj.c */
