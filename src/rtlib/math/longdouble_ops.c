/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: math/longdouble_ops.c

    Purpose:

        Implement arithmetic and conversions for the byte-backed CLongDouble
        type used by FreeBASIC C bindings.

    Responsibilities:

        - preserve the target C compiler's native long double representation
        - provide arithmetic, comparison, and scalar conversion helpers
        - read C long double values from native variadic argument lists
        - reject mismatched storage sizes before reading or writing values

    This file intentionally does NOT contain:

        - FreeBASIC operator declarations
        - compiler parsing or code generation
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <math.h>
#include <float.h>
#if !defined(_WIN32_WCE)
#include <locale.h>
#endif
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FB_LONGDOUBLE_IS_NAN(value) isnan(value)
#include "extended_math.h"
#if defined(__AROS__) || defined(__HAIKU__) || defined(HOST_XBOX) || \
	(defined(__MINGW32__) && !defined(_UCRT) && !defined(_WIN32_WCE))
#define FB_LONGDOUBLE_LOCAL_PARSER
#include "longdouble_parse.h"
#endif

#if defined(__AMIGA__) && defined(__m68k__) && \
	(LDBL_MANT_DIG > DBL_MANT_DIG)
static void longdouble_check_storage(const void *address,
				     unsigned int storage_size);

static void longdouble_store_m68k_strtold(void *destination,
					  unsigned int storage_size,
					  long double value)
{
	uint8_t returned[12];
	uint8_t stored[12];

	/* newlib returns m68k extended values as exponent, significand, padding.
	   GCC's in-memory long-double object uses exponent, padding, significand. */
	longdouble_check_storage(destination, storage_size);
	memcpy(returned, &value, sizeof(returned));
	memcpy(stored, returned, 2);
	memcpy(stored + 2, returned + 10, 2);
	memcpy(stored + 4, returned + 2, 8);
	memcpy(destination, stored, sizeof(stored));
}
#endif

/* CLongDouble comparisons implement exact language equality semantics. */
#if !defined(__m68k__) || (LDBL_MANT_DIG <= DBL_MANT_DIG)
static int longdouble_equal(long double left, long double right)
{
	return (left <= right) && (left >= right);
}
#endif

enum {
	FB_LONGDOUBLE_BOP_ADD = 1,
	FB_LONGDOUBLE_BOP_SUB,
	FB_LONGDOUBLE_BOP_MUL,
	FB_LONGDOUBLE_BOP_DIV
};

enum {
	FB_LONGDOUBLE_UOP_NEG = 1
};

enum {
	FB_LONGDOUBLE_CMP_EQ = 1,
	FB_LONGDOUBLE_CMP_NE,
	FB_LONGDOUBLE_CMP_GT,
	FB_LONGDOUBLE_CMP_LT,
	FB_LONGDOUBLE_CMP_LE,
	FB_LONGDOUBLE_CMP_GE
};

static void longdouble_check_storage(const void *address,
				     unsigned int storage_size)
{
	if ((address == NULL) || (storage_size != sizeof(long double)))
		abort();
}

static long double longdouble_load(const void *source,
				   unsigned int storage_size)
{
	long double value;

	longdouble_check_storage(source, storage_size);
	memcpy(&value, source, sizeof(value));
	return value;
}

static void longdouble_store(void *destination,
			     unsigned int storage_size,
			     long double value)
{
	longdouble_check_storage(destination, storage_size);
	memcpy(destination, &value, sizeof(value));
}

#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
typedef struct FB_M68K_LONGDOUBLE {
	int negative;
	uint16_t exponent;
	uint64_t significand;
} FB_M68K_LONGDOUBLE;

static FB_M68K_LONGDOUBLE longdouble_load_m68k(const void *source,
					       unsigned int storage_size)
{
	const uint8_t *bytes = source;
	FB_M68K_LONGDOUBLE value;
	int byte_index;

	longdouble_check_storage(source, storage_size);
	value.negative = (bytes[0] & UINT8_C(0x80)) != 0;
	value.exponent = (uint16_t)(((uint16_t)(bytes[0] & UINT8_C(0x7f)) << 8) |
				     bytes[1]);
	value.significand = 0;
	for (byte_index = 0; byte_index < 8; byte_index++)
		value.significand = (value.significand << 8) | bytes[4 + byte_index];

	return value;
}

static int longdouble_m68k_is_nan(FB_M68K_LONGDOUBLE value)
{
	return (value.exponent == UINT16_C(0x7fff)) &&
		(value.significand != UINT64_C(0x8000000000000000));
}

static uint64_t longdouble_m68k_to_unsigned(FB_M68K_LONGDOUBLE value)
{
	int exponent;
	int shift;

	if (longdouble_m68k_is_nan(value) || value.negative)
		return 0;
	if (value.exponent == UINT16_C(0x7fff))
		return UINT64_MAX;
	if ((value.exponent == 0) || (value.significand == 0))
		return 0;

	exponent = (int)value.exponent - 16383;
	if (exponent < 0)
		return 0;
	if (exponent >= 64)
		return UINT64_MAX;

	shift = 63 - exponent;
	return value.significand >> shift;
}

static int longdouble_m68k_compare_magnitude(FB_M68K_LONGDOUBLE left,
					    FB_M68K_LONGDOUBLE right)
{
	if (left.exponent < right.exponent)
		return -1;
	if (left.exponent > right.exponent)
		return 1;
	if (left.significand < right.significand)
		return -1;
	if (left.significand > right.significand)
		return 1;
	return 0;
}

static int longdouble_compare_m68k(FB_M68K_LONGDOUBLE left,
				   FB_M68K_LONGDOUBLE right, int operation)
{
	int order;
	int is_unordered = longdouble_m68k_is_nan(left) ||
		longdouble_m68k_is_nan(right);

	if (is_unordered) {
		return operation == FB_LONGDOUBLE_CMP_NE;
	}

	order = longdouble_m68k_compare_magnitude(left, right);
	if (order == 0 && left.exponent == 0 && right.exponent == 0 &&
	    left.significand == 0 && right.significand == 0) {
		left.negative = 0;
		right.negative = 0;
	}
	if (left.negative != right.negative)
		order = left.negative ? -1 : 1;
	else if (left.negative)
		order = -order;

	switch (operation) {
	case FB_LONGDOUBLE_CMP_EQ: return order == 0;
	case FB_LONGDOUBLE_CMP_NE: return order != 0;
	case FB_LONGDOUBLE_CMP_GT: return order > 0;
	case FB_LONGDOUBLE_CMP_LT: return order < 0;
	case FB_LONGDOUBLE_CMP_LE: return order <= 0;
	case FB_LONGDOUBLE_CMP_GE: return order >= 0;
	default: abort();
	}
}

static void longdouble_store_m68k_integer(void *destination, uint64_t value,
					  int negative,
					  unsigned int storage_size)
{
	uint8_t representation[12] = { 0 };
	uint64_t significand = 0;
	uint16_t exponent = 0;
	int highest_bit = 63;
	int byte_index;

	/* GCC's m68k 96-bit extended format stores exponent, two padding bytes,
	 * then the 64-bit significand, all in big-endian order. */
	longdouble_check_storage(destination, storage_size);

	if (value != 0) {
		while (((value >> highest_bit) & UINT64_C(1)) == 0)
			highest_bit--;

		/* Bias 16383 is the IEEE extended-precision exponent bias. */
		exponent = (uint16_t)(16383 + highest_bit);
		significand = value << (63 - highest_bit);
	}

	if (negative)
		exponent |= UINT16_C(0x8000);

	representation[0] = (uint8_t)(exponent >> 8);
	representation[1] = (uint8_t)exponent;
	for (byte_index = 0; byte_index < 8; byte_index++) {
		int shift = 56 - (byte_index * 8);
		representation[4 + byte_index] =
			(uint8_t)(significand >> shift);
	}

	memcpy(destination, representation, sizeof(representation));
}
#endif

void fb_LongDoubleBop(void *destination, const void *lhs, const void *rhs,
		      int operation, unsigned int storage_size)
{
	long double left = longdouble_load(lhs, storage_size);
	long double right = longdouble_load(rhs, storage_size);
	long double result;

	switch (operation) {
	case FB_LONGDOUBLE_BOP_ADD:
		result = left + right;
		break;
	case FB_LONGDOUBLE_BOP_SUB:
		result = left - right;
		break;
	case FB_LONGDOUBLE_BOP_MUL:
		result = left * right;
		break;
	case FB_LONGDOUBLE_BOP_DIV:
		result = left / right;
		break;
	default:
		abort();
	}

	longdouble_store(destination, storage_size, result);
}

void fb_LongDoubleUop(void *destination, const void *source, int operation,
		      unsigned int storage_size)
{
	long double value = longdouble_load(source, storage_size);
	long double result;

	switch (operation) {
	case FB_LONGDOUBLE_UOP_NEG:
		result = -value;
		break;
	default:
		abort();
	}

	longdouble_store(destination, storage_size, result);
}

int fb_LongDoubleCompare(const void *lhs, const void *rhs, int operation,
		 unsigned int storage_size)
{
#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
	return longdouble_compare_m68k(longdouble_load_m68k(lhs, storage_size),
					longdouble_load_m68k(rhs, storage_size),
					operation);
#else
	long double left = longdouble_load(lhs, storage_size);
	long double right = longdouble_load(rhs, storage_size);

	switch (operation) {
	case FB_LONGDOUBLE_CMP_EQ: return longdouble_equal(left, right);
	case FB_LONGDOUBLE_CMP_NE: return !longdouble_equal(left, right);
	case FB_LONGDOUBLE_CMP_GT: return left > right;
	case FB_LONGDOUBLE_CMP_LT: return left < right;
	case FB_LONGDOUBLE_CMP_LE: return left <= right;
	case FB_LONGDOUBLE_CMP_GE: return left >= right;
	default: abort();
	}
#endif
}

void fb_LongDoubleFromSigned(void *destination, int64_t value,
			     unsigned int storage_size)
{
#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
	uint64_t magnitude;
	int negative = value < 0;
	if (negative)
		magnitude = (uint64_t)(-(value + 1)) + 1;
	else
		magnitude = (uint64_t)value;
	longdouble_store_m68k_integer(destination, magnitude, negative,
				      storage_size);
#else
	longdouble_store(destination, storage_size, (long double)value);
#endif
}

void fb_LongDoubleFromUnsigned(void *destination, uint64_t value,
			       unsigned int storage_size)
{
#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
	longdouble_store_m68k_integer(destination, value, 0, storage_size);
#else
	longdouble_store(destination, storage_size, (long double)value);
#endif
}

void fb_LongDoubleFromDouble(void *destination, double value,
			     unsigned int storage_size)
{
	longdouble_store(destination, storage_size, (long double)value);
}

void fb_LongDoubleFromString(void *destination, const char *text,
			     unsigned int storage_size)
{
	long double value;

	if (text == NULL) abort();
#ifdef FB_LONGDOUBLE_LOCAL_PARSER
	/* AROS and Haiku narrow strtold through double. Classic Amiga newlib
	 * returns extended values in a register layout that differs from the C
	 * object's padded memory layout. nxdk has no locale-taking conversion API,
	 * and legacy MinGW/MSVCRT cannot isolate its numeric locale with UCRT's
	 * thread-locale API. The local parser keeps native precision without
	 * depending on those CRT behaviors. */
	size_t consumed;
	value = fb_hParseLongDouble(text, &consumed);
	if ((consumed == 0) || (text[consumed] != '\0')) abort();
#else
	char *end;
#if defined(_WIN32_WCE)
	/* CE's minimal CRT has no locale header or locale objects. Its numeric
	 * conversion uses the C format, and ARM CE has no wider long double. */
#if LDBL_MANT_DIG != DBL_MANT_DIG || LDBL_MAX_EXP != DBL_MAX_EXP
#error Windows CE long double conversion requires the native double format
#endif
	value = (long double)strtod(text, &end);
#elif defined(__MINGW32__) && defined(_UCRT)
	/* UCRT has no locale-taking extended-precision strtold. MinGW's
	 * strtold retains x87 precision; isolate its numeric locale to this
	 * thread, then restore both the category and its previous locale mode. */
	int previous_mode = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
	const char *previous_numeric;
	char *saved_numeric;
	size_t numeric_bytes;
	if (previous_mode == -1) abort();
	previous_numeric = setlocale(LC_NUMERIC, NULL);
	if (previous_numeric == NULL) abort();
	numeric_bytes = strlen(previous_numeric) + 1;
	saved_numeric = malloc(numeric_bytes);
	if (saved_numeric == NULL) abort();
	memcpy(saved_numeric, previous_numeric, numeric_bytes);
	if (setlocale(LC_NUMERIC, "C") == NULL) abort();
	value = strtold(text, &end);
	if (setlocale(LC_NUMERIC, saved_numeric) == NULL) abort();
	free(saved_numeric);
	if (_configthreadlocale(previous_mode) == -1) abort();
#elif defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)
	/* Microsoft's long double has the same representation as double. */
	_locale_t c_locale = _create_locale(LC_NUMERIC, "C");
	if (c_locale == NULL) abort();
	value = (long double)_strtod_l(text, &end, c_locale);
	_free_locale(c_locale);
#elif defined(__NetBSD__)
	/* NetBSD exposes locale-taking conversions, but not POSIX uselocale. */
	locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	if (c_locale == (locale_t)0) abort();
	value = strtold_l(text, &end, c_locale);
	freelocale(c_locale);
/* DJGPP defines __unix__ for compatibility, but has no POSIX locale objects. */
#elif (defined(__unix__) && !defined(__DJGPP__)) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__HAIKU__)
	locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	locale_t previous_locale;
	if (c_locale == (locale_t)0) abort();
	/* strtold_l is not exposed by every BSD/Haiku CRT. POSIX thread locales
	 * give strtold the same C-locale interpretation without changing another
	 * thread's numeric locale. The previous locale is borrowed, not owned. */
	previous_locale = uselocale(c_locale);
	if (previous_locale == (locale_t)0) {
		freelocale(c_locale);
		abort();
	}
	value = strtold(text, &end);
	if (uselocale(previous_locale) == (locale_t)0) abort();
	freelocale(c_locale);
#else
	/* Older targets without locale-specific conversion stay correct in C locale. */
	struct lconv *numeric_locale = localeconv();
	if ((numeric_locale == NULL) || (numeric_locale->decimal_point == NULL) ||
	    (strcmp(numeric_locale->decimal_point, ".") != 0)) abort();
	value = strtold(text, &end);
#endif
	if ((end == text) || (*end != '\0')) abort();
#endif
#if defined(__AMIGA__) && defined(__m68k__) && \
	(LDBL_MANT_DIG > DBL_MANT_DIG)
	longdouble_store_m68k_strtold(destination, storage_size, value);
#else
	longdouble_store(destination, storage_size, value);
#endif
}

int64_t fb_LongDoubleToSigned(const void *source,
			      unsigned int storage_size)
{
#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
	FB_M68K_LONGDOUBLE value = longdouble_load_m68k(source, storage_size);
	uint64_t magnitude;
	int negative = value.negative;
	if (longdouble_m68k_is_nan(value)) return 0;
	value.negative = 0;
	magnitude = longdouble_m68k_to_unsigned(value);
	if (magnitude >= (UINT64_C(1) << 63))
		return negative ? INT64_MIN : INT64_MAX;
	return negative ? -(int64_t)magnitude : (int64_t)magnitude;
#else
	long double value = longdouble_load(source, storage_size);

	/* Avoid undefined C float-to-integer conversions at and beyond the limits. */
	if (FB_LONGDOUBLE_IS_NAN(value)) return 0;
	if (value >= 0x1p63L) return INT64_MAX;
	if (value <= -0x1p63L) return INT64_MIN;
	return (int64_t)value;
#endif
}

uint64_t fb_LongDoubleToUnsigned(const void *source,
				unsigned int storage_size)
{
#if defined(__m68k__) && (LDBL_MANT_DIG > DBL_MANT_DIG)
	return longdouble_m68k_to_unsigned(
		longdouble_load_m68k(source, storage_size));
#else
	long double value = longdouble_load(source, storage_size);

	if (FB_LONGDOUBLE_IS_NAN(value) || (value <= 0.0L)) return 0;
	if (value >= 0x1p64L) return UINT64_MAX;
	return (uint64_t)value;
#endif
}

double fb_LongDoubleToDouble(const void *source,
			     unsigned int storage_size)
{
	return (double)longdouble_load(source, storage_size);
}

void fb_LongDoubleVaArgCopy(void *destination, unsigned int storage_size,
			    va_list arguments)
{
	long double value;

	/* The C compiler knows the target's x87, SSE, or stack va_list layout. */
	longdouble_check_storage(destination, storage_size);
	value = va_arg(arguments, long double);
	memcpy(destination, &value, sizeof(value));
}

long double fb_LongDoubleToNative(const void *source,
				  unsigned int storage_size)
{
	return longdouble_load(source, storage_size);
}

/* end of math/longdouble_ops.c */
