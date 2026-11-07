/*
 * Project: FreeBASIC runtime regression tests
 * File: runtime-array-strings.c
 * Purpose: Observe reverse string retirement and preservation of retained elements.
 * Responsibilities: Check real descriptor deletion, empty storage and prefix bounds.
 * This file intentionally does NOT perform the tested array traversal for the runtime.
 */
#include "fb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sentinels surround the complete descriptor storage, including unused slots. */
typedef struct {
	uint64_t before;
	FBSTRING values[5];
	uint64_t after;
} TEST_STORAGE;

static FBSTRING *active_values;
static size_t active_count;
static size_t active_keep;
static size_t deletion_count;
static size_t previous_index;
static size_t cases;

static void check( int condition, int line )
{
	if( !condition ) {
		fprintf( stderr, "array string retirement check failed at line %d\n", line );
		exit( 1 );
	}
}

#define CHECK(condition) check( (condition), __LINE__ )

FBCALL void __real_fb_StrDelete( FBSTRING *value );
FBCALL void __wrap_fb_StrDelete( FBSTRING *value );

FBCALL void __wrap_fb_StrDelete( FBSTRING *value )
{
	size_t index;
	for( index = 0; index < active_count; ++index ) {
		if( value == &active_values[index] )
			break;
	}
	CHECK( index < active_count );
	CHECK( index >= active_keep );
	CHECK( index < previous_index );
	CHECK( value->data != NULL );
	previous_index = index;
	++deletion_count;
	__real_fb_StrDelete( value );
	CHECK( value->data == NULL );
}

static void run_case( size_t count, size_t keep, unsigned int mask,
	int zero_origin, int public_entry )
{
	TEST_STORAGE storage;
	FBARRAY array;
	char *original[5];
	size_t expected = 0;
	size_t index;

	memset( &storage, 0, sizeof( storage ) );
	memset( &array, 0, sizeof( array ) );
	storage.before = UINT64_C(0x3141592653589793);
	storage.after = UINT64_C(0x2718281828459045);
	for( index = 0; index < 5; ++index ) {
		original[index] = NULL;
		if( index < count && (mask & (1u << index)) != 0 ) {
			storage.values[index].data = malloc( 4 );
			CHECK( storage.values[index].data != NULL );
			memcpy( storage.values[index].data, "abc", 4 );
			storage.values[index].len = 3;
			storage.values[index].size = 3;
			original[index] = storage.values[index].data;
			if( index >= keep )
				++expected;
		}
	}
	/* DATA is an encoded index origin. Retirement must use physical PTR,
	   including when those origin bits are zero. No DATA address is read. */
	array.data = zero_origin ? NULL : storage.values;
	array.ptr = storage.values;
	array.size = count * sizeof( FBSTRING );
	array.element_len = sizeof( FBSTRING );
	array.dimensions = 1;
	array.flags = FBARRAY_FLAGS_FIXED_DIM | FBARRAY_FLAGS_FIXED_LEN | 1u;
	array.dimTB[0].elements = count;
	array.dimTB[0].lbound = 0;
	array.dimTB[0].ubound = count == 0 ? -1 : (ssize_t)count - 1;
	active_values = storage.values;
	active_count = count;
	active_keep = keep;
	deletion_count = 0;
	previous_index = count;
	if( public_entry )
		fb_ArrayDestructStr( &array );
	else
		fb_hArrayDtorStr( &array, NULL, keep );
	CHECK( deletion_count == expected );
	CHECK( array.ptr == storage.values );
	CHECK( array.size == count * sizeof( FBSTRING ) );
	CHECK( storage.before == UINT64_C(0x3141592653589793) );
	CHECK( storage.after == UINT64_C(0x2718281828459045) );
	for( index = 0; index < 5; ++index ) {
		CHECK( storage.values[index].data == (index < count && index < keep ? original[index] : NULL) );
		if( storage.values[index].data != NULL ) {
			CHECK( storage.values[index].len == 3 );
			CHECK( memcmp( storage.values[index].data, "abc", 4 ) == 0 );
			__real_fb_StrDelete( &storage.values[index] );
		}
	}
	++cases;
}

int main( void )
{
	size_t count, keep;
	unsigned int mask;
	int origin;
	FBARRAY empty;
	for( origin = 0; origin < 2; ++origin ) {
		for( count = 0; count <= 4; ++count ) {
			for( mask = 0; mask < (1u << count); ++mask ) {
				for( keep = 0; keep <= count + 1; ++keep )
					run_case( count, keep, mask, origin, 0 );
				run_case( count, SIZE_MAX, mask, origin, 0 );
				run_case( count, 0, mask, origin, 1 );
			}
		}
	}
	/* A null physical pointer must return before reading element_len. */
	memset( &empty, 0, sizeof( empty ) );
	active_count = deletion_count = 0;
	fb_ArrayDestructStr( &empty );
	fb_hArrayDtorStr( &empty, NULL, SIZE_MAX );
	CHECK( deletion_count == 0 );
	cases += 2;
	printf( "STRING_RETIREMENT_PASS cases=%zu\n", cases );
	return 0;
}

/* end of runtime-array-strings.c */
