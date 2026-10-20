/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_destructobj.c
 * Purpose: ERASE for dynamic arrays of objects: destruct elements and free the array.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

void fb_hArrayDtorObj( FBARRAY *array, FB_DEFCTOR dtor, size_t keep_idx )
{
	size_t elements, element_len;
	unsigned char *this_;

	if( array->ptr == NULL )
		return;

	elements = fb_ArrayLen( array );

	/* call dtors in the inverse order */
	element_len = array->element_len;
	this_ = ((unsigned char *)array->ptr) + ((elements-1) * element_len);

	while( elements > keep_idx ) {
		/* !!!FIXME!!! check exceptions (only if rewritten in C++) */
		dtor( this_ );
		this_ -= element_len;
		--elements;
	}
}

FBCALL void fb_ArrayDestructObj( FBARRAY *array, FB_DEFCTOR dtor )
{
	fb_hArrayDtorObj( array, dtor, 0 );
}

/* end of arrays/array_destructobj.c */
