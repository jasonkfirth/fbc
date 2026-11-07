/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_destructobj.c
 * Purpose: Retire object-array elements in reverse order.
 * Responsibilities: Preserve retained elements and valid physical element addresses.
 * This file intentionally does NOT contain: Array allocation or constructor-completion tracking.
 */
#include "fb.h"

void fb_hArrayDtorObj( FBARRAY_ACCESS *array, FB_DEFCTOR dtor, size_t keep_idx )
{
	size_t elements, element_len;
	unsigned char *this_;

	if( array->ptr == NULL )
		return;

	elements = fb_ArrayLen( array );

	/* call dtors in the inverse order */
	element_len = array->element_len;
	this_ = (unsigned char *)array->ptr;

	/* Count down before forming an element address. Keep the physical
	   origin fixed across callbacks, and never step before element zero. */
	while( elements > keep_idx ) {
		--elements;
		/* !!!FIXME!!! check exceptions (only if rewritten in C++) */
		dtor( this_ + (elements * element_len) );
	}
}

FBCALL void fb_ArrayDestructObj( FBARRAY_ACCESS *array, FB_DEFCTOR dtor )
{
	fb_hArrayDtorObj( array, dtor, 0 );
}

/* end of arrays/array_destructobj.c */
