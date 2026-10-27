/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/array-allocation.c
 * Purpose: Check array allocation failure before storage initialization.
 * Responsibilities: Exercise allocation, clearing and constructor paths with
 *     deterministic allocator failures and an observable NULL-fill guard.
 * This file intentionally does NOT test compiler array-index lowering.
 *
 * Compile only arrays/array_redim.c with -DFB_ARRAY_ALLOC_INSTRUMENT and
 * -include tests/rtlib/array-allocation-hooks.h. Compile this fixture without
 * that definition, and link the instrumented
 * object before the remaining runtime archive. The rest of the runtime uses
 * its normal allocators, including error-context initialization.
 *
 * The hooks are single-threaded test state. A NULL fill is recorded instead
 * of executed, so the old implementation fails an assertion without crashing.
 */
#include "array-allocation-hooks.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	ELEMENT_BYTES = 8,
	ELEMENT_COUNT = 3,
	/* Distinguish constructor writes from zero or ASCII-space clearing. */
	CONSTRUCTOR_BYTE = 0x5a
};

typedef struct {
	unsigned char *allocation;
	size_t allocation_size;
	int fail_allocation;
	int malloc_calls;
	int calloc_calls;
	int fill_calls;
	int null_fill_calls;
	int constructor_calls;
} ALLOCATION_PROBE;

static ALLOCATION_PROBE probe;

void *fb_ArrayAllocTestMalloc( size_t size )
{
	++probe.malloc_calls;
	probe.allocation_size = size;
	probe.allocation = probe.fail_allocation ? NULL : malloc( size );
	return probe.allocation;
}

void *fb_ArrayAllocTestCalloc( size_t count, size_t size )
{
	++probe.calloc_calls;
	if( (size != 0) && (count > SIZE_MAX / size) )
		return NULL;
	probe.allocation_size = count * size;
	probe.allocation = probe.fail_allocation ? NULL : calloc( count, size );
	return probe.allocation;
}

void *fb_ArrayAllocTestMemset( void *ptr, int value, size_t size )
{
	++probe.fill_calls;
	if( ptr == NULL ) {
		++probe.null_fill_calls;
		return NULL;
	}
	return memset( ptr, value, size );
}

static void construct_element( void *ptr )
{
	++probe.constructor_calls;
	if( ptr == NULL ) {
		++probe.null_fill_calls;
		return;
	}
	memset( ptr, CONSTRUCTOR_BYTE, ELEMENT_BYTES );
}

/* The allocator consumes bounds through the runtime's va_list interface. */
static int allocate_array( FBARRAY *array, int doclear, FB_DEFCTOR ctor, ... )
{
	va_list ap;
	int result;

	va_start( ap, ctor );
	result = fb_hArrayAlloc( array, ELEMENT_BYTES, doclear, ctor, 1, ap );
	va_end( ap );
	return result;
}

static int check_case( int doclear, int use_constructor, int fail_allocation )
{
	FBARRAY array = { NULL, NULL, 0, ELEMENT_BYTES, 1, 1, {{ 0, 0, 0 }} };
	FB_DEFCTOR ctor = NULL;
	int expected_calloc, result, failures = 0;
	int expected;
	const unsigned char *bytes;
	size_t i;

	probe = (ALLOCATION_PROBE){ NULL, 0, fail_allocation, 0, 0, 0, 0, 0 };
	if( use_constructor )
		ctor = construct_element;
	expected_calloc = (doclear != 0) && (doclear != 32) && !use_constructor;
	result = allocate_array( &array, doclear, ctor,
	                        (ssize_t)0, (ssize_t)(ELEMENT_COUNT - 1) );

	if( (probe.calloc_calls != expected_calloc) ||
	    (probe.malloc_calls != !expected_calloc) ||
	    (probe.allocation_size != ELEMENT_BYTES * ELEMENT_COUNT) )
		++failures;
	if( probe.null_fill_calls != 0 )
		++failures;

	if( fail_allocation ) {
		if( (result != FB_RTERROR_OUTOFMEM) ||
		    (fb_ErrorGetNum() != FB_RTERROR_OUTOFMEM) ||
		    (array.ptr != NULL) || (array.data != NULL) ||
		    (probe.constructor_calls != 0) )
			++failures;
	} else {
		if( (result != FB_RTERROR_OK) || (fb_ErrorGetNum() != FB_RTERROR_OK) ||
		    (array.ptr == NULL) || (array.ptr != probe.allocation) ||
		    (array.data != array.ptr) ||
		    (array.size != ELEMENT_BYTES * ELEMENT_COUNT) ||
		    (array.element_len != ELEMENT_BYTES) || (array.dimensions != 1) ||
		    (array.dimTB[0].elements != ELEMENT_COUNT) ||
		    (array.dimTB[0].lbound != 0) ||
		    (array.dimTB[0].ubound != ELEMENT_COUNT - 1) ||
		    (probe.constructor_calls != (use_constructor ? ELEMENT_COUNT : 0)) ||
		    (probe.fill_calls != ((doclear == 32) && !use_constructor)) )
			++failures;

		/* Uninitialized malloc storage has no value contract to inspect. */
		if( (array.ptr != NULL) && (use_constructor || (doclear != 0)) ) {
			bytes = array.ptr;
			expected = use_constructor ? CONSTRUCTOR_BYTE : (doclear == 32 ? 32 : 0);
			for( i = 0; i < ELEMENT_BYTES * ELEMENT_COUNT; ++i ) {
				if( bytes[i] != expected ) {
					++failures;
					break;
				}
			}
		}
	}

	free( array.ptr );
	if( failures != 0 )
		fprintf( stderr, "array allocation: clear=%d ctor=%d fail=%d checks=%d null_fills=%d\n",
		         doclear, use_constructor, fail_allocation, failures, probe.null_fill_calls );
	return failures;
}

int main( void )
{
	/* 32 requests ASCII-space fill; other nonzero modes request zero fill. */
	static const int clear_modes[] = { 0, 1, 32, -1 };
	size_t mode;
	int use_constructor, fail_allocation;
	int cases = 0, failures = 0, null_fills = 0;

	fb_Init( 0, NULL, 0 );
	for( mode = 0; mode < sizeof( clear_modes ) / sizeof( clear_modes[0] ); ++mode ) {
		for( use_constructor = 0; use_constructor <= 1; ++use_constructor ) {
			for( fail_allocation = 0; fail_allocation <= 1; ++fail_allocation ) {
				failures += check_case( clear_modes[mode], use_constructor, fail_allocation );
				null_fills += probe.null_fill_calls;
				++cases;
			}
		}
	}
	printf( "ARRAY_ALLOCATION_CASES=%d NULL_FILL_ATTEMPTS=%d FAILURES=%d\n",
	        cases, null_fills, failures );
	return failures != 0;
}

/* end of rtlib/array-allocation.c */
