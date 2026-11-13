/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_destructstr.c
 * Purpose: ERASE for dynamic arrays of var-len strings.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

void fb_hArrayDtorStr( FBARRAY_ACCESS *array, FB_DEFCTOR dtor, size_t keep_idx )
{
	size_t elements;
	FBSTRING_ACCESS *this_;
	/* String elements use fb_StrDelete instead of the generic destructor. */
	(void)dtor;

	if( array->ptr == NULL )
		return;

	elements = fb_ArrayLen( array );

	/* Count down before forming an element address. Decrementing the
	   pointer after element zero would leave the physical allocation. */
	while( elements > keep_idx ) {
		--elements;
		this_ = (FBSTRING_ACCESS *)array->ptr + elements;
		if( this_->data != NULL )
			fb_StrDelete( this_ );
	}
}

FBCALL void fb_ArrayDestructStr( FBARRAY_ACCESS *array )
{
	fb_hArrayDtorStr( array, NULL, 0 );
}

/* end of arrays/array_destructstr.c */
