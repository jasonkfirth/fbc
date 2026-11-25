/*
 * Project: FreeBASIC runtime library
 * File: arrays/array_redimto.c
 * Purpose: Initialize array storage from an existing descriptor.
 * Responsibilities: Array descriptors, bounds, allocation and element lifetimes.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_ArrayRedimTo
	(
		FBARRAY_ACCESS *dest,
		const FBARRAY_ACCESS *source,
		int isvarlen,
		FB_DEFCTOR ctor,
		FB_DEFCTOR dtor
	)
{
	ssize_t diff;
	unsigned char *this_;
	unsigned char *limit;

	if( dest == source ) {
		return fb_ErrorSetNum( FB_RTERROR_OK );
	}

	/* ditto, see fb_hArrayAlloc() */
	if( (source->dimensions != dest->dimensions) && (dest->dimensions != 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	/* Empty arrays have no storage bias. Retrieve nonempty bias before
	   destructors run, preserving the original callback ordering. */
	diff = 0;
	if( source->size != 0 ) {
		diff = (ssize_t)((uintptr_t)source->data - (uintptr_t)source->ptr);
	}

	/* free old */
	if( dtor )
		fb_ArrayDestructObj( dest, dtor );

	if( isvarlen ) {
		fb_ArrayStrErase( dest );
	} else {
		fb_ArrayErase( dest );
	}

	DBG_ASSERT( dest->element_len == source->element_len || dest->element_len == 0 );
	DBG_ASSERT( dest->dimensions == source->dimensions || dest->dimensions == 0 );

	/* Copy over bounds etc. */
	dest->size = source->size;
	dest->element_len = source->element_len;
	dest->dimensions = source->dimensions;
	memcpy( &dest->dimTB[0], &source->dimTB[0], sizeof( FBARRAYDIM ) * dest->dimensions );

	/* Empty/unallocated source array? */
	if( dest->size == 0 ) {
		/* Destination will be empty/unallocated too */
		dest->ptr = NULL;
		dest->data = NULL;
		return fb_ErrorSetNum( FB_RTERROR_OK );
	}

	/* Allocate new buffer; clear unless ctors will be called.
	   (ctors take care of clearing themselves) */
	if( ctor == NULL ) {
		dest->ptr = calloc( dest->size, 1 );
	} else {
		dest->ptr = malloc( dest->size );
	}
	if( dest->ptr == NULL ) {
		/* Keep the failed descriptor empty if it is copied again. */
		fb_ArrayResetDesc( dest );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	dest->data = (void *)((uintptr_t)dest->ptr + (uintptr_t)diff);

	/* Call ctor for each element */
	if( ctor ) {
		this_ = dest->ptr;
		limit = this_ + dest->size;
		while( this_ < limit ) {
			ctor( this_ );
			this_ += dest->element_len;
		}
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* Initialize a fresh descriptor for a BYVAL parameter. The destination
   owns its allocation, even when the source has fixed storage. For elements
   that need construction, generated code publishes the byte count as each
   element becomes live so an unwind only destroys constructed elements. */
FBCALL int fb_ArrayCopyDesc
	(
		FBARRAY_ACCESS *dest,
		const FBARRAY_ACCESS *source,
		int copy_bytes
	)
{
	size_t dimensions = source->dimensions;
	void *storage;

	if( dimensions == 0 )
		return FB_RTERROR_OK;

	if( (dimensions > (source->flags & FBARRAY_FLAGS_DIMENSIONS)) ||
	    (dimensions > (dest->flags & FBARRAY_FLAGS_DIMENSIONS)) ||
	    ((dest->dimensions != 0) && (dest->dimensions != dimensions)) ||
	    (dest->element_len != source->element_len) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	storage = NULL;
	if( source->size != 0 ) {
		if( (source->ptr == NULL) || (source->element_len == 0) ||
		    (source->size % source->element_len != 0) )
			return FB_RTERROR_ILLEGALFUNCTIONCALL;

		storage = copy_bytes ? malloc( source->size ) : calloc( 1, source->size );
		if( storage == NULL )
			return FB_RTERROR_OUTOFMEM;
	}

	dest->dimensions = dimensions;
	memcpy( &dest->dimTB[0], &source->dimTB[0], dimensions * sizeof( FBARRAYDIM ) );
	dest->ptr = storage;

	if( storage != NULL ) {
		uintptr_t bias = (uintptr_t)source->data - (uintptr_t)source->ptr;
		dest->data = (void *)((uintptr_t)storage + bias);
		if( copy_bytes ) {
			memcpy( storage, source->ptr, source->size );
			dest->size = source->size;
		}
	}

	return FB_RTERROR_OK;
}

/* end of arrays/array_redimto.c */
