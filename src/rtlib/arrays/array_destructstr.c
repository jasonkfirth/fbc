/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_destructstr.c
 * Purpose: ERASE for dynamic arrays of var-len strings.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

void fb_hArrayDtorStr( FBARRAY *array, FB_DEFCTOR dtor, size_t keep_idx )
{
	size_t elements;
	FBSTRING *this_;

	if( array->ptr == NULL )
		return;

	elements = fb_ArrayLen( array );

	/* call dtors in the inverse order */
	this_ = (FBSTRING *)array->ptr + (elements-1);

	while( elements > keep_idx ) {
		if( this_->data != NULL )
			fb_StrDelete( this_ );
		--this_;
		--elements;
	}
}

FBCALL void fb_ArrayDestructStr( FBARRAY *array )
{
	fb_hArrayDtorStr( array, NULL, 0 );
}

/* end of arrays/array_destructstr.c */
