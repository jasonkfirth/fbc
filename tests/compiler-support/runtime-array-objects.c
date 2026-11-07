/*
 * Project: FreeBASIC runtime regression tests
 * File: runtime-array-objects.c
 * Purpose: Observe reverse object retirement through the real array helpers.
 * Responsibilities: Check callback order, retained storage, ownership and metadata.
 * This file intentionally does NOT perform the runtime's destructor traversal.
 */
#include "fb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_ELEMENTS = 5, STORAGE_BYTES = 192 };

/* Five 33-byte elements fit with spare bytes. The complete byte buffer is
   a multiple of eight, keeping both 64-bit sentinels naturally aligned. */
typedef struct {
	uint64_t before;
	unsigned char values[STORAGE_BYTES];
	uint64_t after;
} TEST_STORAGE;

/* Cases are sequential. The observer owns payloads independently of the array;
   each destructor must release exactly the payload belonging to its address. */
static unsigned char *active_values;
static char *owned[MAX_ELEMENTS];
static size_t active_count, active_keep, active_stride, previous_index;
static size_t destruction_count, cases;

static void check( int condition, int line )
{
	if( !condition ) {
		fprintf( stderr, "array object retirement check failed at line %d\n", line );
		exit( 1 );
	}
}

#define CHECK(condition) check( (condition), __LINE__ )

static void destroy( void *value )
{
	size_t index;
	unsigned char *bytes = value;
	for( index = 0; index < active_count; ++index ) {
		if( value == active_values + index * active_stride )
			break;
	}
	CHECK( index < active_count );
	CHECK( index >= active_keep );
	CHECK( index + 1 == previous_index );
	CHECK( bytes[0] == (unsigned char)(0x20u + index) );
	CHECK( owned[index] != NULL );
	CHECK( memcmp( owned[index], "obj", 4 ) == 0 );
	free( owned[index] );
	owned[index] = NULL;
	bytes[0] = (unsigned char)(0xd0u + index);
	previous_index = index;
	++destruction_count;
}

static void run_case( size_t count, size_t keep, size_t stride,
	int zero_origin, int public_entry )
{
	TEST_STORAGE storage;
	FBARRAY array, original;
	size_t index, byte, expected;
	CHECK( count <= MAX_ELEMENTS && stride != 0 );
	CHECK( count * stride <= sizeof( storage.values ) );
	memset( &storage, 0, sizeof( storage ) );
	memset( &array, 0, sizeof( array ) );
	memset( storage.values, 0xa5, sizeof( storage.values ) );
	storage.before = UINT64_C(0x3141592653589793);
	storage.after = UINT64_C(0x2718281828459045);
	for( index = 0; index < MAX_ELEMENTS; ++index ) {
		owned[index] = NULL;
		if( index < count ) {
			owned[index] = malloc( 4 );
			CHECK( owned[index] != NULL );
			memcpy( owned[index], "obj", 4 );
			memset( storage.values + index * stride, (int)(0x20u + index), stride );
		}
	}
	/* DATA is only an encoded index origin. Physical PTR remains authoritative,
	   including when the virtual origin bits happen to be zero. */
	array.data = zero_origin ? NULL : storage.values;
	array.ptr = storage.values;
	array.size = count * stride;
	array.element_len = stride;
	array.dimensions = 1;
	array.flags = FBARRAY_FLAGS_FIXED_DIM | FBARRAY_FLAGS_FIXED_LEN | 1u;
	array.dimTB[0].elements = count;
	array.dimTB[0].lbound = 0;
	array.dimTB[0].ubound = count == 0 ? -1 : (ssize_t)count - 1;
	memcpy( &original, &array, sizeof( original ) );
	active_values = storage.values;
	active_count = count;
	active_keep = keep;
	active_stride = stride;
	previous_index = count;
	destruction_count = 0;
	if( public_entry )
		fb_ArrayDestructObj( &array, destroy );
	else
		fb_hArrayDtorObj( &array, destroy, keep );
	expected = keep < count ? count - keep : 0;
	CHECK( destruction_count == expected );
	CHECK( memcmp( &array, &original, sizeof( array ) ) == 0 );
	CHECK( storage.before == UINT64_C(0x3141592653589793) );
	CHECK( storage.after == UINT64_C(0x2718281828459045) );
	for( index = 0; index < count; ++index ) {
		CHECK( (owned[index] != NULL) == (index < keep) );
		for( byte = 0; byte < stride; ++byte ) {
			unsigned char expected_byte = (unsigned char)(0x20u + index);
			if( index >= keep && byte == 0 )
				expected_byte = (unsigned char)(0xd0u + index);
			CHECK( storage.values[index * stride + byte] == expected_byte );
		}
		free( owned[index] );
		owned[index] = NULL;
	}
	for( byte = count * stride; byte < sizeof( storage.values ); ++byte )
		CHECK( storage.values[byte] == 0xa5 );
	++cases;
}

int main( void )
{
	const size_t strides[] = { 1, 3, 7, 16, 33 };
	size_t count, keep, stride;
	int origin;
	FBARRAY empty;
	for( origin = 0; origin < 2; ++origin ) {
		for( stride = 0; stride < sizeof( strides ) / sizeof( strides[0] ); ++stride ) {
			for( count = 0; count <= MAX_ELEMENTS; ++count ) {
				for( keep = 0; keep <= count + 1; ++keep )
					run_case( count, keep, strides[stride], origin, 0 );
				run_case( count, SIZE_MAX - 1, strides[stride], origin, 0 );
				run_case( count, SIZE_MAX, strides[stride], origin, 0 );
				run_case( count, 0, strides[stride], origin, 1 );
			}
		}
	}
	/* An unallocated descriptor returns before element_len is consulted. */
	memset( &empty, 0, sizeof( empty ) );
	destruction_count = 0;
	fb_ArrayDestructObj( &empty, destroy );
	fb_hArrayDtorObj( &empty, destroy, SIZE_MAX );
	CHECK( destruction_count == 0 );
	cases += 2;
	printf( "OBJECT_RETIREMENT_PASS cases=%zu\n", cases );
	return 0;
}

/* end of runtime-array-objects.c */
