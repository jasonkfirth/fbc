/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_lbound.c
 * Purpose: lbound function.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* Returns the lbound of the given dimension, or 0 if the given dimension
   doesn't exist. Together with ubound() returning -1 in such cases, we'll have
   the lbound > ubound situation to detect unallocated arrays.

   Using lbound == 0 for unallocated arrays is also good because it allows FB
   code such as
      @array(lbound(array)) <> NULL
   to keep working.

   Special case: lbound( a, 0 ) always returns 1 (the lbound of the dimTB).
   Together with ubound() returning the dimension count or 0, we'll also have
   the lbound > ubound situation here for unallocated arrays.

   Note: The dimension count can be set (in the descriptor) even for unallocated
   arrays, as it's fixed and determines the descriptor size. */
FBCALL ssize_t fb_ArrayLBound( FBARRAY_ACCESS *array, ssize_t dimension )
{
	/* Keep the dimension 1-based until it has been validated below. */

	/* Querying dimTB's lbound? */
	if( dimension == 0 ) {
		return 1;
	}

	/* Querying dimension's lbound. */

	/* PTR identifies storage; virtual DATA can have zero address bits.
	   Validate the 1-based dimension before subtracting, even at SSIZE_MIN. */
	/* unallocated array or out-of-bound dimension? */
	if( (array->ptr == NULL) ||
	    (dimension < 1) || ((size_t)dimension > array->dimensions) ) {
		return 0;
	}
	return array->dimTB[dimension - 1].lbound;
}

/* end of arrays/array_lbound.c */
