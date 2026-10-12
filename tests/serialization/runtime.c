/* FreeBASIC serialization runtime tests
 * File: runtime.c
 * Purpose: Check hostile input, allocation failures and byte-level encoding.
 * Responsibilities: Independent wire fixtures and ownership accounting.
 * This file intentionally contains no implementation of the codec.
 */

#include "fb.h"
#include "fb_serialization.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>

static void *owned[128];
static size_t live, attempts, fail_at = SIZE_MAX;

/* Only the codec object is compiled with allocator substitution. The actual
 * runtime's descriptor/string pool remains in use, so ABI/lifetime mistakes
 * cannot be hidden behind mock string services. */
void *ser_test_calloc( size_t count, size_t bytes )
{
	void *ptr;
	if( attempts++ == fail_at ) return NULL;
	ptr = calloc(count, bytes);
	if( ptr ) { assert(live < 128); owned[live++] = ptr; }
	return ptr;
}

void *ser_test_realloc( void *old, size_t bytes )
{
	size_t i;
	void *ptr;
	if( attempts++ == fail_at ) return NULL;
	for( i = 0; i < live; ++i ) if( owned[i] == old ) break;
	ptr = realloc(old, bytes);
	if( ptr ) {
		if( i == live ) { assert(live < 128); ++live; }
		owned[i] = ptr;
	}
	return ptr;
}

void ser_test_free( void *ptr )
{
	size_t i;
	for( i = 0; i < live; ++i ) if( owned[i] == ptr ) { owned[i] = owned[--live]; break; }
	free(ptr);
}

static FBSTRING text( char *bytes )
{
	FBSTRING result = {bytes, (ssize_t)strlen(bytes), (ssize_t)strlen(bytes)};
	return result;
}

static FBSTRING fixture( const char *schema, const unsigned char *body, size_t bytes )
{
	size_t length = strlen(schema);
	FBSTRING result = {0};
	result.len = result.size = (ssize_t)(8 + length + bytes);
	result.data = malloc((size_t)result.len);
	assert(result.data);
	memcpy(result.data, "FBS1", 4);
	result.data[4] = (char)(length & 255);
	result.data[5] = (char)((length >> 8) & 255);
	result.data[6] = result.data[7] = 0;
	memcpy(result.data + 8, schema, length);
	if( bytes ) memcpy(result.data + 8 + length, body, bytes);
	return result;
}

static void release_result( FBSTRING *result )
{
	ser_test_free(result->data);
	result->data = NULL;
	fb_hStrDelTemp(result);
}

typedef struct {
	void *data, *ptr;
	size_t size, element_len, dimensions, flags;
	FBARRAYDIM dimTB[1];
} TEST_ARRAY;

typedef struct { FBSTRING label_text; TEST_ARRAY values; } TEST_RECORD;

static void packed_roundtrip( void )
{
	unsigned char source[1 + sizeof(FBSTRING) + sizeof(TEST_ARRAY)] = {0};
	unsigned char destination[sizeof(source)] = {0};
	char program_text[256];
	FBSTRING source_text = text("packed"), restored_text, payload = {0};
	TEST_ARRAY array = {0}, restored_array;
	FBSTRING schema = text("packed"), program, *encoded;
	ssize_t numbers[2] = {-17, 23};

	array.ptr = malloc(sizeof(numbers));
	assert(array.ptr);
	memcpy(array.ptr, numbers, sizeof(numbers));
	array.data = (unsigned char *)array.ptr + sizeof(ssize_t);
	array.size = sizeof(numbers); array.element_len = sizeof(ssize_t);
	array.dimensions = 1; array.flags = 1 | FBARRAY_FLAGS_FIXED_DIM;
	array.dimTB[0].elements = 2; array.dimTB[0].lbound = -1; array.dimTB[0].ubound = 0;
	source[0] = 9;
	memcpy(source + 1, &source_text, sizeof(source_text));
	memcpy(source + 1 + sizeof(source_text), &array, sizeof(array));
	snprintf(program_text, sizeof(program_text), "C,%u,0,3,0;U,1,0,0,0;S,%u,1,0,0;R,%u,%u,1,1;N,%u,0,0,0;",
	    (unsigned)sizeof(source), (unsigned)sizeof(FBSTRING), (unsigned)sizeof(ssize_t),
	    (unsigned)(1 + sizeof(FBSTRING)), (unsigned)sizeof(ssize_t));
	program = text(program_text);
	encoded = fb_Serialize(source, &schema, &program);
	assert(FB_STRSIZE(encoded) > 0);
	payload.len = payload.size = FB_STRSIZE(encoded);
	payload.data = malloc((size_t)payload.len);
	assert(payload.data);
	memcpy(payload.data, encoded->data, (size_t)payload.len);
	release_result(encoded);
	assert(fb_Deserialize(destination, &schema, &program, &payload));
	assert(destination[0] == 9);
	memcpy(&restored_text, destination + 1, sizeof(restored_text));
	memcpy(&restored_array, destination + 1 + sizeof(restored_text), sizeof(restored_array));
	assert(restored_text.len == 6 && !memcmp(restored_text.data, "packed", 6));
	assert(restored_text.data != source_text.data);
	assert(restored_array.ptr != array.ptr && !memcmp(restored_array.ptr, numbers, sizeof(numbers)));
	assert(restored_array.dimTB[0].lbound == -1 && restored_array.dimTB[0].ubound == 0);
	ser_test_free(restored_text.data);
	ser_test_free(restored_array.ptr);
	free(payload.data);
	free(array.ptr);
	assert(live == 0);
}

static int reject( void *destination, const char *schema_text, const char *program_text,
                   const unsigned char *body, size_t bytes )
{
	FBSTRING schema = text((char *)schema_text), program = text((char *)program_text);
	FBSTRING payload = fixture(schema_text, body, bytes);
	int result = fb_Deserialize(destination, &schema, &program, &payload) == 0;
	free(payload.data);
	assert(live == 0);
	assert(fb_ErrorGetNum() == FB_RTERROR_ILLEGALFUNCTIONCALL);
	return result;
}

static void uint64_bytes( unsigned char *p, uint64_t value )
{
	size_t i;
	for( i = 0; i < 8; ++i ) { p[i] = (unsigned char)value; value >>= 8; }
}

int main( void )
{
	char native_program[80], record_program[512], array_program[128];
	ssize_t destination = 17;
	unsigned char body[64] = {0};
	FBSTRING schema, program, payload, *encoded;
	size_t i, failures = 0;
	TEST_ARRAY array = {0};
	TEST_RECORD record = {0}, before;

	packed_roundtrip();

	snprintf(native_program, sizeof(native_program), "N,%u,0,0,0;", (unsigned)sizeof(ssize_t));
	uint64_bytes(body, UINT64_C(0xfffffffffffffffe));
	schema = text("N"); program = text(native_program);
	payload = fixture("N", body, 8);
	assert(fb_Deserialize(&destination, &schema, &program, &payload));
	assert(destination == -2 && live == 0);
	encoded = fb_Serialize(&destination, &schema, &program);
	assert(FB_STRSIZE(encoded) > 0 && (size_t)FB_STRSIZE(encoded) == (size_t)payload.len);
	assert(!memcmp(encoded->data, payload.data, (size_t)payload.len));
	release_result(encoded);
	free(payload.data);
	assert(live == 0);
	if( sizeof(ssize_t) == 4 ) {
		uint64_bytes(body, UINT64_C(0x0000000080000000));
		assert(reject(&destination, "N", native_program, body, 8));
		assert(destination == -2);
	}

	body[0] = 2;
	{ unsigned char flag = 1; assert(reject(&flag, "B", "B,1,0,0,0;", body, 1)); assert(flag == 1); }
	{
		FBSTRING string = {0};
		char string_program[80];
		snprintf(string_program, sizeof(string_program), "S,%u,0,0,0;", (unsigned)sizeof(FBSTRING));
		memset(body, 255, 4);
		assert(reject(&string, "S", string_program, body, 4));
	}

	snprintf(array_program, sizeof(array_program), "R,1,0,0,1;U,1,0,0,0;");
	array.dimensions = 1; array.element_len = 1; array.flags = 1 | FBARRAY_FLAGS_FIXED_DIM;
	body[0] = 9;
	assert(reject(&array, "R1[U1]", array_program, body, 1));
	body[0] = 1;
	uint64_bytes(body + 1, 0); uint64_bytes(body + 9, UINT64_MAX >> 1);
	assert(reject(&array, "R1[U1]", array_program, body, 17));
	uint64_bytes(body + 1, UINT64_C(0x8000000000000000));
	uint64_bytes(body + 9, UINT64_C(0x8000000000000000));
	assert(reject(&array, "R1[U1]", array_program, body, 18));
	uint64_bytes(body + 1, 5); uint64_bytes(body + 9, 4);
	assert(reject(&array, "R1[U1]", array_program, body, 17));
	uint64_bytes(body + 1, 0); uint64_bytes(body + 9, 1048576);
	assert(reject(&array, "R1[U1]", array_program, body, 17));
	assert(array.ptr == NULL && array.dimensions == 1);
	/* Even zero-wire-size records cannot use array bounds to allocate more
	 * than the aggregate memory budget or bypass the traversal work budget. */
	body[0] = 1; uint64_bytes(body + 1, 0); uint64_bytes(body + 9, 65535);
	assert(reject(&array, "large", "R,1024,0,0,1;C,1024,0,0,0;", body, 17));
	uint64_bytes(body + 9, 1048575);
	assert(reject(&array, "work", "R,1,0,0,1;C,1,0,0,0;", body, 17));

	for( i = 0; i < 2; ++i ) {
		attempts = 0; fail_at = i;
		schema = text("N"); program = text(native_program);
		encoded = fb_Serialize(&destination, &schema, &program);
		assert(FB_STRSIZE(encoded) == 0 && fb_ErrorGetNum() == FB_RTERROR_OUTOFMEM && live == 0);
	}
	fail_at = SIZE_MAX;
	/* Source STRING function results also need cleanup when the first codec
	 * allocation fails, before a parsed root node is available. */
	{
		char string_program[80];
		FBSTRING *source = fb_hStrAllocTemp(NULL, 3);
		assert(source);
		memcpy(source->data, "abc", 4);
		snprintf(string_program, sizeof(string_program), "S,%u,0,0,0;", (unsigned)sizeof(FBSTRING));
		schema = text("S"); program = text(string_program);
		attempts = 0; fail_at = 0;
		encoded = fb_Serialize(source, &schema, &program);
		assert(FB_STRSIZE(encoded) == 0 && fb_ErrorGetNum() == FB_RTERROR_OUTOFMEM && live == 0);
		assert(source->data == NULL);
		fail_at = SIZE_MAX;
	}
	{
		FBSTRING *descriptors[FB_STR_TMPDESCRIPTORS];
		size_t count = 0;
		while( count < FB_STR_TMPDESCRIPTORS && (descriptors[count] = fb_hStrAllocTempDesc()) != NULL ) ++count;
		schema = text("N"); program = text(native_program);
		encoded = fb_Serialize(&destination, &schema, &program);
		assert(FB_STRSIZE(encoded) == 0 && fb_ErrorGetNum() == FB_RTERROR_OUTOFMEM && live == 0);
		while( count ) fb_hStrDelTempDesc(descriptors[--count]);
	}

	/* Fail every codec allocation while restoring two strings in an array.
	 * A byte-for-byte destination guard complements the logical FB tests. */
	snprintf(record_program, sizeof(record_program), "C,%u,0,2,0;S,%u,%u,0,0;R,%u,%u,0,1;S,%u,0,0,0;",
	    (unsigned)sizeof(TEST_RECORD), (unsigned)sizeof(FBSTRING), (unsigned)offsetof(TEST_RECORD,label_text),
	    (unsigned)sizeof(FBSTRING), (unsigned)offsetof(TEST_RECORD,values), (unsigned)sizeof(FBSTRING));
	memset(body, 0, sizeof(body));
	body[0] = 3; memcpy(body + 4, "abc", 3);
	body[7] = 1; uint64_bytes(body + 8, 0); uint64_bytes(body + 16, 1);
	body[24] = 1; body[28] = 'x'; body[29] = 1; body[33] = 'y';
	schema = text("C{LABEL_TEXT=S;VALUES=R1[S];}"); program = text(record_program);
	payload = fixture(schema.data, body, 34);
	record.label_text = text("keep");
	record.label_text.data = malloc(5); memcpy(record.label_text.data, "keep", 5);
	record.values.element_len = sizeof(FBSTRING); record.values.dimensions = 1;
	record.values.flags = 1 | FBARRAY_FLAGS_FIXED_DIM;
	before = record;
	for( i = 0; i < 12; ++i ) {
		attempts = 0; fail_at = i;
		if( fb_Deserialize(&record, &schema, &program, &payload) ) {
			assert(i == attempts);
			assert(record.label_text.len == 3 && !memcmp(record.label_text.data, "abc", 3));
			assert(record.values.size == 2 * sizeof(FBSTRING));
			ser_test_free(record.label_text.data);
			ser_test_free(((FBSTRING *)record.values.ptr)[0].data);
			ser_test_free(((FBSTRING *)record.values.ptr)[1].data);
			ser_test_free(record.values.ptr);
			assert(live == 0);
			break;
		}
		++failures;
		assert(fb_ErrorGetNum() == FB_RTERROR_OUTOFMEM);
		assert(!memcmp(&record, &before, sizeof(record)));
		assert(!memcmp(record.label_text.data, "keep", 5));
		assert(live == 0);
	}
	assert(failures >= 5);
	free(payload.data);
	fail_at = SIZE_MAX;

	/* A deterministic mutation sweep exercises invalid lengths, schemas,
	 * versions, dimensions and bounds after successful nested allocation. */
	payload = fixture(schema.data, body, 34);
	memset(&record, 0, sizeof(record));
	record.values.element_len = sizeof(FBSTRING); record.values.dimensions = 1;
	record.values.flags = 1 | FBARRAY_FLAGS_FIXED_DIM;
	for( i = 0; i < (size_t)payload.len; ++i ) {
		unsigned char original = (unsigned char)payload.data[i];
		payload.data[i] = (char)(original ^ 0x80);
		if( fb_Deserialize(&record, &schema, &program, &payload) ) {
			if( record.label_text.data ) ser_test_free(record.label_text.data);
			if( record.values.ptr ) {
				size_t j;
				for( j = 0; j < record.values.size / sizeof(FBSTRING); ++j ) ser_test_free(((FBSTRING *)record.values.ptr)[j].data);
				ser_test_free(record.values.ptr);
			}
			memset(&record, 0, sizeof(record));
		}
		assert(live == 0);
		payload.data[i] = (char)original;
	}
	free(payload.data);
	printf("SERIALIZATION_RUNTIME=PASS BITS=%u ALLOCATION_FAILURES=%u\n", (unsigned)(sizeof(void *) * 8), (unsigned)failures);
	return 0;
}

/* end of runtime.c */
