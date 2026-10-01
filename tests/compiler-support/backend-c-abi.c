/*
 * Project: FreeBASIC compiler regression tests
 * File: backend-c-abi.c
 * Purpose: Check large aggregate arguments across a real C ABI boundary.
 * Responsibilities: Consume a value copy and invoke a BASIC callback with it.
 * This file intentionally does NOT contain compiler or platform ABI selection.
 */

#include <stdint.h>

struct compiler_support_aggregate {
	int64_t a;
	int64_t b;
	int64_t c;
	int64_t d;
};

struct __attribute__((packed)) compiler_support_packed_aggregate {
	/* Match BASIC FIELD = 1: tag at byte 0, followed by values at 1, 9, 17. */
	uint8_t tag;
	int64_t a;
	int64_t b;
	int64_t c;
};

int64_t compiler_support_c_packed_aggregate(
	struct compiler_support_packed_aggregate value)
{
	return value.tag + value.a * 10 + value.b * 100 + value.c * 1000;
}

int64_t compiler_support_c_aggregate(struct compiler_support_aggregate value)
{
	return value.a + value.b * 10 + value.c * 100 + value.d * 1000;
}

int64_t compiler_support_c_callback(
	int64_t (*callback)(struct compiler_support_aggregate),
	struct compiler_support_aggregate value)
{
	return callback(value);
}

/* end of backend-c-abi.c */
