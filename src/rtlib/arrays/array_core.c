/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_core.c
 * Purpose: dynamic arrays core.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* Calculate the number of array elements based on the bounds passed
   to fb_hArrayRealloc(). This differs from array->dimTB[]. */
size_t fb_hArrayCalcElements
	(
		size_t dimensions,
		const ssize_t *lboundTB,
		const ssize_t *uboundTB
	)
{
	size_t i, elements;

	if( (dimensions == 0) || (dimensions > FB_MAXDIMENSIONS) )
		return 0;

	elements = 1;
	for( i = 0; i < dimensions; i++ ) {
		size_t dim_elements = fb_hArrayCalcDimElements( lboundTB[i], uboundTB[i] );

		if( (dim_elements == 0) || (elements > SIZE_MAX / dim_elements) )
			return 0;

		elements *= dim_elements;
	}

	return elements;
}

ssize_t fb_hArrayCalcDiff
	(
		size_t dimensions,
		const ssize_t *lboundTB,
		const ssize_t *uboundTB
	)
{
	size_t i, elements;
	size_t diff = 0;

	if( (dimensions == 0) || (dimensions > FB_MAXDIMENSIONS) )
		return 0;

	for( i = 0; i < dimensions-1; i++ )
	{
		elements = fb_hArrayCalcDimElements( lboundTB[i+1], uboundTB[i+1] );
		if( elements == 0 )
			return 0;
		diff = (diff + (size_t)lboundTB[i]) * elements;
	}

	diff += (size_t)lboundTB[dimensions-1];

	/* A virtual origin wraps in the native address width. */
	return (ssize_t)((size_t)0 - diff);
}

FBCALL size_t fb_ArrayLen( FBARRAY_ACCESS *array )
{
	return (array && array->ptr)
		? array->size / array->element_len : 0;

/*

	Previously, the number of elements was computed from
	the array descriptor's dimensions table.

	{
		size_t i, elements;
		FBARRAYDIM *dim;

		dim = &array->dimTB[0];
		elements = dim->elements;
		++dim;

		for( i = 1; i < array->dimensions; i++, dim++ )
			elements *= dim->elements;

		return elements;
	}
*/
}

FBCALL size_t fb_ArraySize( FBARRAY_ACCESS *array )
{
	return (array && array->ptr)
		? array->size : 0;
}

/* end of arrays/array_core.c */
