/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_ubound.c
 * Purpose: ubound function.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* Returns the ubound of the given dimension, or -1 if the given dimension
   doesn't exist.

   Special case: ubound( a, 0 ) returns the dimension count.

   Note: The dimension count can be set (in the descriptor) even for unallocated
   arrays, as it's fixed and determines the descriptor size. However, currently
   ubound( a, 0 ) will always return 0 for unallocated arrays. */
FBCALL ssize_t fb_ArrayUBound( FBARRAY_ACCESS *array, ssize_t dimension )
{
	/* Keep the dimension 1-based until it has been validated below. */

	/* Querying dimension count? */
	if( dimension == 0 ) {
		if( array->ptr ) {
			return (ssize_t)array->dimensions;
		}
		return 0;
	}

	/* Querying dimension's ubound. */

	/* PTR identifies storage; virtual DATA can have zero address bits.
	   Validate the 1-based dimension before subtracting, even at SSIZE_MIN. */
	/* unallocated array or out-of-bound dimension? */
	if( (array->ptr == NULL) ||
	    (dimension < 1) || ((size_t)dimension > array->dimensions) ) {
		return -1;
	}
	return array->dimTB[dimension - 1].ubound;
}

/* end of arrays/array_ubound.c */
