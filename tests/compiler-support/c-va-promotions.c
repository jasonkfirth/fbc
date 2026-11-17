/*
 * Project: FreeBASIC C backend regression
 * File: c-va-promotions.c
 * Purpose: Observe promoted arguments and compare native formatted values.
 * Responsibilities: Check byte ranges, short boundaries and repeated calls.
 * BASIC owns the calls under test and its independent variadic reader.
 */
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int fb_test_promotion_native(int, int, ...);
int fb_test_promotion_basic(int, int, ...);
int fb_test_promotion_format(char *, size_t, unsigned char, unsigned short);
int fb_test_promotion_cases(void);

static unsigned calls;

int fb_test_promotion_native(int expected_byte, int expected_short, ...)
{
	va_list args;
	int actual_byte;
	int actual_short;
	va_start(args, expected_short);
	actual_byte = va_arg(args, int);
	actual_short = va_arg(args, int);
	va_end(args);
	++calls;
	return actual_byte == expected_byte && actual_short == expected_short ? -1 : 0;
}

int fb_test_promotion_cases(void)
{
	static const unsigned short boundaries[] = {0, 1, 127, 128, 255, 256, 32767, 32768, USHRT_MAX};
	char actual[96];
	char expected[96];
	unsigned comparisons = 0;
	calls = 0;
	for (unsigned byte = 0; byte <= UCHAR_MAX; ++byte) {
		for (size_t index = 0; index < sizeof(boundaries) / sizeof(boundaries[0]); ++index) {
			unsigned char byte_value = (unsigned char)byte;
			unsigned short short_value = boundaries[index];
			int written = fb_test_promotion_format(actual, sizeof(actual), byte_value, short_value);
			int reference = snprintf(expected, sizeof(expected), "%x %o %hx %ho",
				byte_value, byte_value, short_value, short_value);
			if (written != reference || strcmp(actual, expected) != 0)
				return 0;
			if (fb_test_promotion_basic(byte_value, short_value, byte_value, short_value) != -1)
				return 0;
			++comparisons;
		}
	}
	return comparisons == 2304 && calls == 4608 ? -1 : 0;
}

/* end of c-va-promotions.c */
