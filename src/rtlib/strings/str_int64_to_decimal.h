/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: strings/str_int64_to_decimal.h

    Purpose:

        Convert signed and unsigned 64-bit integers to decimal text without
        relying on a particular C runtime's printf length modifiers.

    Responsibilities:

        - format the full 64-bit range into a caller-provided buffer
        - report insufficient output capacity without writing a partial value

    This file intentionally does NOT contain:

        - FreeBASIC string allocation or ownership
        - locale-dependent numeric formatting
        - floating-point conversion
*/

#ifndef FB_STR_INT64_TO_DECIMAL_H
#define FB_STR_INT64_TO_DECIMAL_H

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Decimal conversion                                                       */
/* ------------------------------------------------------------------------- */

static int fb_hInt64MagnitudeToDecimal( char *destination, size_t capacity,
                                        uint64_t magnitude, int negative )
{
	char reversed[20];
	size_t digits = 0;
	size_t output_chars;
	size_t index;
	size_t offset;

	if( destination == NULL )
		return -1;

	do {
		reversed[digits++] = (char)('0' + (magnitude % UINT64_C(10)));
		magnitude /= UINT64_C(10);
	} while( magnitude != 0 );

	output_chars = digits + (negative ? 1u : 0u);
	if( capacity <= output_chars )
		return -1;

	offset = 0;
	if( negative )
		destination[offset++] = '-';

	for( index = 0; index < digits; ++index )
		destination[offset + index] = reversed[digits - index - 1u];

	destination[output_chars] = '\0';
	return (int)output_chars;
}

static int fb_hInt64ToDecimal( char *destination, size_t capacity, int64_t value )
{
	int negative = (value < 0);
	uint64_t magnitude = (uint64_t)value;

	/* Unsigned subtraction obtains INT64_MIN's magnitude without signed overflow. */
	if( negative )
		magnitude = UINT64_C(0) - magnitude;

	return fb_hInt64MagnitudeToDecimal(destination, capacity, magnitude, negative);
}

static int fb_hUInt64ToDecimal( char *destination, size_t capacity, uint64_t value )
{
	return fb_hInt64MagnitudeToDecimal(destination, capacity, value, 0);
}

#endif

/* end of strings/str_int64_to_decimal.h */
