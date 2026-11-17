/*
 * Project: FreeBASIC console qualification
 * File: c-format-probe.c
 * Purpose: Provide an independent native C baseline for variadic formatting.
 * Responsibilities: Call the SDK formatter with C's own promoted argument ABI.
 * This file intentionally does NOT replace libc formatting or compiler output.
 */
#include <stdio.h>
#include <stddef.h>

int fbqual_sprintf( char *output, size_t capacity )
{
	volatile int signed_value = -123;
	volatile unsigned int unsigned_value = 4000000000u;
	volatile double real_value = 1.25;

	/* The fixed format's result fits this caller-owned buffer. Volatile
	   inputs retain a real variadic SDK call rather than a constant fold. */
	if( output == NULL || capacity < 128 ) return -1;
	return sprintf(output, "%d %u %.2f %s", signed_value, unsigned_value, real_value, "text");
}

/* end of c-format-probe.c */
