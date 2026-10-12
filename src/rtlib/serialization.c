/*
 * FreeBASIC runtime
 * File: serialization.c
 * Purpose: Execute compiler-generated logical serialization programs.
 * Responsibilities: Portable encoding, bounded decoding, deep ownership and
 *     transactional replacement of ordinary values and array descriptors.
 * This file intentionally contains no transport, reflection registry, object
 *     constructors or pointer ownership inference.
 */

#include "fb.h"
#include "fb_serialization.h"
#include <float.h>
#include <limits.h>
#include <stddef.h>

/* Limits apply to each call, including aggregate allocations/work. A payload
 * cannot request arbitrary descriptor dimensions or recursion depth. */
#define SER_BYTES (16u * 1024u * 1024u)
#define SER_MEMORY (64u * 1024u * 1024u)
#define SER_SCHEMA 65536u
#define SER_NODES 4096u
#define SER_DEPTH 32u
#define SER_WORK 1048576u

typedef struct {
	char kind;
	size_t size, offset, count, dims, storage, child, next, minimum;
	int64_t lower[FB_MAXDIMENSIONS], upper[FB_MAXDIMENSIONS];
} SER_NODE;

/* Match FBARRAY's header and reserve the maximum descriptor capacity. Values
 * can be packed UDT fields, so descriptors are always accessed with memcpy. */
typedef struct {
	void *data, *ptr;
	size_t size, element_len, dimensions, flags;
	FBARRAYDIM dimTB[FB_MAXDIMENSIONS];
} SER_ARRAY;

typedef struct {
	SER_NODE *nodes;
	size_t used, node_capacity, work, allocated, length, capacity, position;
	unsigned char *buffer;
	const unsigned char *input;
	int error;
} SER_CONTEXT;

/* ------------------------------------------------------------------------- */
/* Compiler program and checked resource accounting                           */
/* ------------------------------------------------------------------------- */

static int ser_fail( SER_CONTEXT *ctx, int error )
{
	if( !ctx->error ) ctx->error = error;
	return 0;
}

static void *ser_alloc( SER_CONTEXT *ctx, size_t bytes )
{
	void *ptr;
	if( bytes > SER_MEMORY - ctx->allocated ) {
		ser_fail( ctx, FB_RTERROR_ILLEGALFUNCTIONCALL );
		return NULL;
	}
	ptr = calloc( bytes ? bytes : 1, 1 );
	if( !ptr ) ser_fail( ctx, FB_RTERROR_OUTOFMEM );
	else ctx->allocated += bytes;
	return ptr;
}

static int ser_number( const char **cursor, const char *end, char delimiter, int64_t *value )
{
	const char *p = *cursor;
	uint64_t magnitude = 0, limit;
	int negative = 0, digits = 0;
	while( p < end && *p == ' ' ) ++p;
	if( p < end && *p == '-' ) { negative = 1; ++p; }
	limit = negative ? UINT64_C(9223372036854775808) : INT64_MAX;
	while( p < end && *p >= '0' && *p <= '9' ) {
		unsigned digit = (unsigned)(*p++ - '0');
		if( magnitude > (limit - digit) / 10 ) return 0;
		magnitude = magnitude * 10 + digit;
		digits = 1;
	}
	if( !digits || p == end || *p++ != delimiter ) return 0;
	if( negative ) *value = magnitude == UINT64_C(9223372036854775808) ? INT64_MIN : -(int64_t)magnitude;
	else *value = (int64_t)magnitude;
	*cursor = p;
	return 1;
}

static size_t ser_program( SER_CONTEXT *ctx, const char **cursor, const char *end, size_t depth )
{
	SER_NODE *node;
	size_t index, i, child;
	int64_t values[5];
	if( depth >= SER_DEPTH || ctx->used == ctx->node_capacity || end - *cursor < 2 ) goto invalid;
	index = ctx->used++;
	node = &ctx->nodes[index];
	node->kind = *(*cursor)++;
	if( *(*cursor)++ != ',' ) goto invalid;
	for( i = 0; i < 4; ++i ) {
		if( !ser_number(cursor, end, i == 3 ? ';' : ',', &values[i]) || values[i] < 0 ) goto invalid;
	}
	if( values[0] > SER_MEMORY || values[1] > SER_MEMORY || values[2] > SER_WORK || values[3] > FB_MAXDIMENSIONS ) goto invalid;
	node->size = (size_t)values[0]; node->offset = (size_t)values[1];
	node->count = (size_t)values[2]; node->dims = (size_t)values[3];
	node->storage = node->size;
	node->minimum = node->size;
	node->child = ctx->used;
	switch( node->kind ) {
	case 'C':
		node->minimum = 0;
		for( i = 0; i < node->count; ++i ) {
			child = ser_program(ctx, cursor, end, depth + 1);
			if( ctx->error ) return 0;
			if( ctx->nodes[child].offset > node->size || ctx->nodes[child].storage > node->size - ctx->nodes[child].offset ) goto invalid;
			if( ctx->nodes[child].minimum > SER_BYTES - node->minimum ) goto invalid;
			node->minimum += ctx->nodes[child].minimum;
		}
		break;
	case 'A':
	case 'R':
		if( !node->dims || !node->size ) goto invalid;
		if( node->kind == 'R' && node->count > node->dims ) goto invalid;
		if( node->kind == 'A' ) {
			size_t count = 1;
			for( i = 0; i < node->dims; ++i ) {
				uint64_t extent;
				if( !ser_number(cursor, end, ',', &node->lower[i]) || !ser_number(cursor, end, ';', &node->upper[i]) || node->lower[i] > node->upper[i] ) goto invalid;
				extent = (uint64_t)node->upper[i] - (uint64_t)node->lower[i];
				if( extent >= SER_WORK || count > SER_WORK / (extent + 1) ) goto invalid;
				count *= (size_t)(extent + 1);
			}
			if( count != node->count || count > SER_MEMORY / node->size ) goto invalid;
			node->storage = count * node->size;
		} else node->storage = offsetof(SER_ARRAY, dimTB) + node->dims * sizeof(FBARRAYDIM);
		child = ser_program(ctx, cursor, end, depth + 1);
		if( ctx->error ) return 0;
		if( ctx->nodes[child].offset || ctx->nodes[child].storage != node->size ) goto invalid;
		if( node->kind == 'A' ) {
			if( ctx->nodes[child].minimum && node->count > SER_BYTES / ctx->nodes[child].minimum ) goto invalid;
			node->minimum = node->count * ctx->nodes[child].minimum;
		} else node->minimum = 1;
		break;
	case 'S':
		if( node->size != sizeof(FBSTRING) ) goto invalid;
		node->minimum = 4;
		break;
	case 'B':
		if( node->size != 1 ) goto invalid;
		break;
	case 'N': case 'V':
		if( node->size != sizeof(ssize_t) ) goto invalid;
		node->minimum = 8;
		break;
	case 'I': case 'U':
		if( node->size != 1 && node->size != 2 && node->size != 4 && node->size != 8 ) goto invalid;
		break;
	case 'F':
		if( node->size != 4 || sizeof(float) != 4 || FLT_RADIX != 2 || FLT_MANT_DIG != 24 ) goto invalid;
		break;
	case 'D':
		if( node->size != 8 || sizeof(double) != 8 || DBL_MANT_DIG != 53 ) goto invalid;
		break;
	case 'T': break;
	default: goto invalid;
	}
	node->next = ctx->used;
	return index;
invalid:
	ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	return 0;
}

static int ser_prepare( SER_CONTEXT *ctx, FBSTRING *schema, FBSTRING *program )
{
	const char *cursor, *end, *p;
	if( !schema || !program || !schema->data || !program->data || FB_STRSIZE(schema) <= 0 ||
	    FB_STRSIZE(schema) > SER_SCHEMA || FB_STRSIZE(program) <= 0 || FB_STRSIZE(program) > SER_BYTES )
		return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	cursor = program->data;
	end = cursor + FB_STRSIZE(program);
	/* Count compiler instructions before allocating. A primitive needs one
	 * node, not a maximum-sized table. Bounds contain digits, never opcodes. */
	for( p = cursor; p + 1 < end; ++p ) {
		if( *p >= 'A' && *p <= 'Z' && p[1] == ',' ) ++ctx->node_capacity;
	}
	if( !ctx->node_capacity || ctx->node_capacity > SER_NODES ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	ctx->nodes = ser_alloc(ctx, ctx->node_capacity * sizeof(SER_NODE));
	if( !ctx->nodes ) return 0;
	ser_program(ctx, &cursor, end, 0);
	if( cursor != end || ctx->nodes[0].offset ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	return !ctx->error;
}

/* ------------------------------------------------------------------------- */
/* Wire primitives: explicit little endian, never native descriptor bytes     */
/* ------------------------------------------------------------------------- */

static int ser_put( SER_CONTEXT *ctx, const void *data, size_t bytes )
{
	size_t capacity;
	unsigned char *buffer;
	if( bytes > SER_BYTES - ctx->length ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	if( ctx->length + bytes > ctx->capacity ) {
		capacity = ctx->capacity ? ctx->capacity : 256;
		while( capacity < ctx->length + bytes ) capacity *= 2;
		buffer = realloc(ctx->buffer, capacity + 1);
		if( !buffer ) return ser_fail(ctx, FB_RTERROR_OUTOFMEM);
		ctx->buffer = buffer;
		ctx->capacity = capacity;
	}
	if( bytes ) memcpy(ctx->buffer + ctx->length, data, bytes);
	ctx->length += bytes;
	return 1;
}

static int ser_get( SER_CONTEXT *ctx, void *data, size_t bytes )
{
	if( bytes > ctx->length - ctx->position ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	if( bytes ) memcpy(data, ctx->input + ctx->position, bytes);
	ctx->position += bytes;
	return 1;
}

static int ser_put_uint( SER_CONTEXT *ctx, uint64_t value, size_t bytes )
{
	unsigned char buffer[8];
	size_t i;
	for( i = 0; i < bytes; ++i ) { buffer[i] = (unsigned char)value; value >>= 8; }
	return ser_put(ctx, buffer, bytes);
}

static int ser_get_uint( SER_CONTEXT *ctx, uint64_t *value, size_t bytes )
{
	unsigned char buffer[8];
	size_t i;
	if( !ser_get(ctx, buffer, bytes) ) return 0;
	*value = 0;
	for( i = 0; i < bytes; ++i ) *value |= (uint64_t)buffer[i] << (i * 8);
	return 1;
}

static uint64_t ser_native_uint( const void *ptr, size_t bytes )
{
	uint8_t a; uint16_t b; uint32_t c; uint64_t d;
	switch( bytes ) {
	case 1: memcpy(&a, ptr, 1); return a;
	case 2: memcpy(&b, ptr, 2); return b;
	case 4: memcpy(&c, ptr, 4); return c;
	default: memcpy(&d, ptr, 8); return d;
	}
}

static void ser_store_uint( void *ptr, uint64_t value, size_t bytes )
{
	uint8_t a = (uint8_t)value; uint16_t b = (uint16_t)value;
	uint32_t c = (uint32_t)value;
	switch( bytes ) {
	case 1: memcpy(ptr, &a, 1); break;
	case 2: memcpy(ptr, &b, 2); break;
	case 4: memcpy(ptr, &c, 4); break;
	default: memcpy(ptr, &value, 8); break;
	}
}

/* Both FB integer storage and array bounds use two's complement. memcpy
 * avoids implementation-defined conversions when the high bit is set. */
static int64_t ser_signed( uint64_t value )
{
	int64_t result;
	memcpy(&result, &value, 8);
	return result;
}

static int ser_array_shape( SER_CONTEXT *ctx, const SER_NODE *node, SER_ARRAY *array, size_t *count )
{
	size_t i, elements = 1;
	int64_t bias = 0;
	for( i = 0; i < array->dimensions; ++i ) {
		int64_t lower = array->dimTB[i].lbound, upper = array->dimTB[i].ubound;
		uint64_t extent;
		if( lower > upper ) goto invalid;
		extent = (uint64_t)upper - (uint64_t)lower;
		if( extent >= SER_WORK || elements > SER_WORK / (extent + 1) ) goto invalid;
		array->dimTB[i].elements = (size_t)(extent + 1);
		elements *= (size_t)(extent + 1);
		/* Bias is calculated with checked signed arithmetic before negation
		 * or scaling. This is the row-major FBARRAY.data indexing convention. */
		if( i ) {
			int64_t factor = (int64_t)array->dimTB[i].elements;
			if( bias > INT64_MAX / factor || bias < INT64_MIN / factor ) goto invalid;
			bias *= factor;
		}
		if( (lower > 0 && bias > INT64_MAX - lower) || (lower < 0 && bias < INT64_MIN - lower) ) goto invalid;
		bias += lower;
	}
	if( elements > SER_MEMORY / node->size ) goto invalid;
	if( bias == INT64_MIN || bias > INT64_MAX / (int64_t)node->size || bias < -INT64_MAX / (int64_t)node->size ) goto invalid;
	bias = -bias * (int64_t)node->size;
	if( bias < INTPTR_MIN || bias > INTPTR_MAX ) goto invalid;
	/* FB uses a biased address which may lie outside the allocation. Integer
	 * address arithmetic avoids forming an out-of-bounds C pointer by addition. */
	array->data = (void *)((uintptr_t)array->ptr + (uintptr_t)bias);
	*count = elements;
	return 1;
invalid:
	return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
}

static int ser_load_array( SER_CONTEXT *ctx, const void *ptr, SER_ARRAY *array )
{
	size_t capacity;
	memset(array, 0, sizeof(*array));
	memcpy(array, ptr, offsetof(SER_ARRAY, dimTB));
	capacity = array->flags & FBARRAY_FLAGS_DIMENSIONS;
	if( capacity > FB_MAXDIMENSIONS || array->dimensions > capacity )
		return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	/* A BYDESC parameter with unknown rank may refer to a caller's small
	 * descriptor. Read its actual dimensions, never the parameter's capacity. */
	memcpy(array->dimTB, (const unsigned char *)ptr + offsetof(SER_ARRAY, dimTB),
	       array->dimensions * sizeof(FBARRAYDIM));
	return 1;
}

/* ------------------------------------------------------------------------- */
/* Logical traversal and owned temporary reconstruction                       */
/* ------------------------------------------------------------------------- */

static int ser_write( SER_CONTEXT *ctx, size_t index, const unsigned char *base )
{
	const SER_NODE *node = &ctx->nodes[index];
	const unsigned char *ptr = base + node->offset;
	size_t i, child, count;
	uint64_t value;
	FBSTRING string;
	SER_ARRAY array;
	if( ++ctx->work > SER_WORK ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	switch( node->kind ) {
	case 'C':
		child = node->child;
		for( i = 0; i < node->count; ++i ) {
			if( !ser_write(ctx, child, ptr) ) return 0;
			child = ctx->nodes[child].next;
		}
		return 1;
	case 'A':
		for( i = 0; i < node->count; ++i ) if( !ser_write(ctx, node->child, ptr + i * node->size) ) return 0;
		return 1;
	case 'R':
		if( !ser_load_array(ctx, ptr, &array) ) return 0;
		if( !array.ptr ) return ser_put_uint(ctx, 0, 1);
		if( !array.dimensions || array.dimensions > node->dims || (node->count && array.dimensions != node->count) || array.element_len != node->size ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		if( !ser_array_shape(ctx, node, &array, &count) || array.size != count * node->size ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		if( !ser_put_uint(ctx, array.dimensions, 1) ) return 0;
		for( i = 0; i < array.dimensions; ++i ) {
			if( !ser_put_uint(ctx, (uint64_t)array.dimTB[i].lbound, 8) || !ser_put_uint(ctx, (uint64_t)array.dimTB[i].ubound, 8) ) return 0;
		}
		for( i = 0; i < count; ++i ) if( !ser_write(ctx, node->child, (unsigned char *)array.ptr + i * node->size) ) return 0;
		return 1;
	case 'S':
		memcpy(&string, ptr, sizeof(string));
		if( FB_STRSIZE(&string) < 0 || FB_STRSIZE(&string) > SER_BYTES || (FB_STRSIZE(&string) && !string.data) ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		return ser_put_uint(ctx, (uint64_t)FB_STRSIZE(&string), 4) && ser_put(ctx, string.data, (size_t)FB_STRSIZE(&string));
	case 'T': return ser_put(ctx, ptr, node->size);
	default:
		value = ser_native_uint(ptr, node->size);
		if( node->kind == 'B' ) value = value ? 1 : 0;
		if( node->kind == 'N' && node->size < 8 && (value & (UINT64_C(1) << (node->size * 8 - 1))) ) value |= UINT64_MAX << (node->size * 8);
		return ser_put_uint(ctx, value, node->minimum);
	}
}

static void ser_destroy( SER_CONTEXT *ctx, size_t index, unsigned char *base )
{
	const SER_NODE *node = &ctx->nodes[index];
	unsigned char *ptr = base + node->offset;
	size_t i, child;
	FBSTRING string;
	SER_ARRAY array;
	switch( node->kind ) {
	case 'C':
		child = node->child;
		for( i = 0; i < node->count; ++i ) { ser_destroy(ctx, child, ptr); child = ctx->nodes[child].next; }
		break;
	case 'A':
		for( i = 0; i < node->count; ++i ) ser_destroy(ctx, node->child, ptr + i * node->size);
		break;
	case 'S':
		memcpy(&string, ptr, sizeof(string));
		free(string.data);
		break;
	case 'R':
		if( !ser_load_array(ctx, ptr, &array) ) break;
		if( array.ptr ) {
			for( i = 0; i < array.size / node->size; ++i ) ser_destroy(ctx, node->child, (unsigned char *)array.ptr + i * node->size);
			free(array.ptr);
		}
		break;
	}
}

static int ser_read( SER_CONTEXT *ctx, size_t index, unsigned char *base )
{
	const SER_NODE *node = &ctx->nodes[index];
	unsigned char *ptr = base + node->offset;
	size_t i, child, count;
	uint64_t value;
	FBSTRING string = {0};
	SER_ARRAY array = {0};
	if( ++ctx->work > SER_WORK ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	switch( node->kind ) {
	case 'C':
		child = node->child;
		for( i = 0; i < node->count; ++i ) {
			if( !ser_read(ctx, child, ptr) ) return 0;
			child = ctx->nodes[child].next;
		}
		return 1;
	case 'A':
		for( i = 0; i < node->count; ++i ) if( !ser_read(ctx, node->child, ptr + i * node->size) ) return 0;
		return 1;
	case 'R':
		array.element_len = node->size;
		array.flags = node->dims | (node->count ? FBARRAY_FLAGS_FIXED_DIM : 0);
		if( !ser_get_uint(ctx, &value, 1) ) return 0;
		if( value > node->dims || (value && node->count && value != node->count) ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		array.dimensions = value ? (size_t)value : node->count;
		if( !value ) { memcpy(ptr, &array, node->storage); return 1; }
		for( i = 0; i < array.dimensions; ++i ) {
			if( !ser_get_uint(ctx, &value, 8) ) return 0;
			if( ser_signed(value) < INTPTR_MIN || ser_signed(value) > INTPTR_MAX ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
			array.dimTB[i].lbound = (ssize_t)ser_signed(value);
			if( !ser_get_uint(ctx, &value, 8) ) return 0;
			if( ser_signed(value) < INTPTR_MIN || ser_signed(value) > INTPTR_MAX ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
			array.dimTB[i].ubound = (ssize_t)ser_signed(value);
		}
		if( !ser_array_shape(ctx, node, &array, &count) ) return 0;
		if( ctx->nodes[node->child].minimum && count > (ctx->length - ctx->position) / ctx->nodes[node->child].minimum ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		array.size = count * node->size;
		array.ptr = ser_alloc(ctx, array.size);
		if( !array.ptr ) return 0;
		ser_array_shape(ctx, node, &array, &count);
		/* Publish into scratch before visiting children so failure cleanup can
		 * free this allocation and any successfully decoded child allocations. */
		memcpy(ptr, &array, node->storage);
		for( i = 0; i < count; ++i ) if( !ser_read(ctx, node->child, (unsigned char *)array.ptr + i * node->size) ) return 0;
		return 1;
	case 'S':
		if( !ser_get_uint(ctx, &value, 4) ) return 0;
		if( value > ctx->length - ctx->position ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		if( value ) {
			string.data = ser_alloc(ctx, (size_t)value + 1);
			if( !string.data ) return 0;
			string.len = string.size = (ssize_t)value;
			ser_get(ctx, string.data, (size_t)value);
		}
		memcpy(ptr, &string, sizeof(string));
		return 1;
	case 'T': return ser_get(ctx, ptr, node->size);
	default:
		if( !ser_get_uint(ctx, &value, node->minimum) ) return 0;
		if( node->kind == 'B' ) {
			if( value > 1 ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
			/* BOOLEAN storage is 0/1, although BASIC comparisons produce -1. */
		}
		if( node->kind == 'N' && node->size < 8 && (ser_signed(value) < INT32_MIN || ser_signed(value) > INT32_MAX) ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		if( node->kind == 'V' && node->size < 8 && value > UINT32_MAX ) return ser_fail(ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
		ser_store_uint(ptr, value, node->size);
		return 1;
	}
}

/* ------------------------------------------------------------------------- */
/* Entry points and transactional commit                                      */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_Serialize( void *value, FBSTRING *schema, FBSTRING *program )
{
	SER_CONTEXT ctx = {0};
	FBSTRING result = {0}, *temporary = &__fb_ctx.null_desc;
	if( value && ser_prepare(&ctx, schema, program) && ser_put(&ctx, "FBS1", 4) &&
	    ser_put_uint(&ctx, (uint64_t)FB_STRSIZE(schema), 4) &&
	    ser_put(&ctx, schema->data, (size_t)FB_STRSIZE(schema)) && ser_write(&ctx, 0, value) ) {
		ctx.buffer[ctx.length] = 0;
		result.data = (char *)ctx.buffer;
		result.len = (ssize_t)ctx.length;
		result.size = (ssize_t)ctx.capacity;
		temporary = fb_StrAllocTempResult(&result);
		if( result.data ) { free(result.data); ser_fail(&ctx, FB_RTERROR_OUTOFMEM); }
	} else {
		free(ctx.buffer);
		if( !ctx.error ) ser_fail(&ctx, FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	/* BYREF AS ANY forwards a STRING function result's temporary descriptor
	 * directly, without converting it to a scope-owned STRING variable. This
	 * entry point must consume that temporary. The root opcode identifies it even
	 * if allocating the node table failed. Ordinary/packed STRING variables
	 * are read with memcpy and never passed to alignment-dependent cleanup. */
	if( value && program && program->data && FB_STRSIZE(program) >= 2 &&
	    program->data[0] == 'S' && program->data[1] == ',' ) {
		FBSTRING source;
		memcpy(&source, value, sizeof(source));
		if( FB_ISTEMP(&source) ) fb_hStrDelTemp(value);
	}
	free(ctx.nodes);
	fb_hStrDelTemp(schema);
	fb_hStrDelTemp(program);
	fb_ErrorSetNum(ctx.error);
	return temporary;
}

FBCALL ssize_t fb_Deserialize( void *value, FBSTRING *schema, FBSTRING *program, FBSTRING *payload )
{
	SER_CONTEXT ctx = {0};
	unsigned char *scratch = NULL;
	uint64_t length;
	int success = 0;
	if( !value || !payload || FB_STRSIZE(payload) < 8 || FB_STRSIZE(payload) > SER_BYTES || !payload->data ) goto done;
	if( !ser_prepare(&ctx, schema, program) ) goto done;
	ctx.input = (unsigned char *)payload->data;
	ctx.length = (size_t)FB_STRSIZE(payload);
	ctx.position = 4;
	if( memcmp(ctx.input, "FBS1", 4) || !ser_get_uint(&ctx, &length, 4) || length != (uint64_t)FB_STRSIZE(schema) || length > ctx.length - ctx.position ) goto done;
	if( memcmp(ctx.input + ctx.position, schema->data, (size_t)length) ) goto done;
	ctx.position += (size_t)length;
	if( ctx.nodes[0].minimum > ctx.length - ctx.position ) goto done;
	scratch = ser_alloc(&ctx, ctx.nodes[0].storage);
	if( !scratch || !ser_read(&ctx, 0, scratch) || ctx.position != ctx.length ) goto done;
	if( ctx.nodes[0].kind == 'R' ) {
		SER_ARRAY old = {0}, replacement = {0};
		size_t i;
		if( !ser_load_array(&ctx, value, &old) ) goto done;
		memcpy(&replacement, scratch, ctx.nodes[0].storage);
		if( replacement.dimensions > (old.flags & FBARRAY_FLAGS_DIMENSIONS) ) goto done;
		if( old.flags & FBARRAY_FLAGS_FIXED_DIM ) {
			if( replacement.ptr && replacement.dimensions != old.dimensions ) goto done;
			if( !replacement.ptr ) replacement.dimensions = old.dimensions;
		}
		replacement.flags = old.flags;
		if( old.flags & FBARRAY_FLAGS_FIXED_LEN ) {
			if( old.size != replacement.size || old.dimensions != replacement.dimensions || !replacement.ptr ) goto done;
			for( i = 0; i < old.dimensions; ++i ) {
				if( old.dimTB[i].lbound != replacement.dimTB[i].lbound || old.dimTB[i].ubound != replacement.dimTB[i].ubound ) goto done;
			}
			/* A fixed array's descriptor and backing store belong to its caller.
			 * Replace elements while moving the temporary's deep allocations. */
			for( i = 0; i < old.size / ctx.nodes[0].size; ++i ) ser_destroy(&ctx, ctx.nodes[0].child, (unsigned char *)old.ptr + i * ctx.nodes[0].size);
			memcpy(old.ptr, replacement.ptr, old.size);
			free(replacement.ptr);
		} else {
			ser_destroy(&ctx, 0, value);
			memcpy(value, &replacement, offsetof(SER_ARRAY, dimTB) + replacement.dimensions * sizeof(FBARRAYDIM));
		}
	} else {
		ser_destroy(&ctx, 0, value);
		memcpy(value, scratch, ctx.nodes[0].storage);
	}
	free(scratch);
	scratch = NULL;
	success = 1;
done:
	if( scratch ) { ser_destroy(&ctx, 0, scratch); free(scratch); }
	free(ctx.nodes);
	fb_hStrDelTemp(schema);
	fb_hStrDelTemp(program);
	fb_hStrDelTemp(payload);
	if( !success && !ctx.error ) ctx.error = FB_RTERROR_ILLEGALFUNCTIONCALL;
	fb_ErrorSetNum(ctx.error);
	return success ? FB_TRUE : FB_FALSE;
}

FBCALL FBSTRING *fb_SerializeArray( FBARRAY *value, FBSTRING *schema, FBSTRING *program )
{
	return fb_Serialize(value, schema, program);
}

FBCALL ssize_t fb_DeserializeArray( FBARRAY *value, FBSTRING *schema, FBSTRING *program, FBSTRING *payload )
{
	return fb_Deserialize(value, schema, program, payload);
}

/* end of serialization.c */
