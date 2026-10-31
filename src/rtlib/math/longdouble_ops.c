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
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
	long double left = longdouble_load(lhs, storage_size);
	long double right = longdouble_load(rhs, storage_size);

	switch (operation) {
	case FB_LONGDOUBLE_CMP_EQ: return left == right;
	case FB_LONGDOUBLE_CMP_NE: return left != right;
	case FB_LONGDOUBLE_CMP_GT: return left > right;
	case FB_LONGDOUBLE_CMP_LT: return left < right;
	case FB_LONGDOUBLE_CMP_LE: return left <= right;
	case FB_LONGDOUBLE_CMP_GE: return left >= right;
	default: abort();
	}
}

void fb_LongDoubleFromSigned(void *destination, int64_t value,
			     unsigned int storage_size)
{
	longdouble_store(destination, storage_size, (long double)value);
}

void fb_LongDoubleFromUnsigned(void *destination, uint64_t value,
			       unsigned int storage_size)
{
	longdouble_store(destination, storage_size, (long double)value);
}

void fb_LongDoubleFromDouble(void *destination, double value,
			     unsigned int storage_size)
{
	longdouble_store(destination, storage_size, (long double)value);
}

void fb_LongDoubleFromString(void *destination, const char *text,
			     unsigned int storage_size)
{
	char *end;
	long double value;

	if (text == NULL) abort();
#if defined(__MINGW32__) && defined(_UCRT)
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
#elif defined(__unix__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__HAIKU__)
	locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
	if (c_locale == (locale_t)0) abort();
	value = strtold_l(text, &end, c_locale);
	freelocale(c_locale);
#else
	/* Older targets without locale-specific conversion stay correct in C locale. */
	struct lconv *numeric_locale = localeconv();
	if ((numeric_locale == NULL) || (numeric_locale->decimal_point == NULL) ||
	    (strcmp(numeric_locale->decimal_point, ".") != 0)) abort();
	value = strtold(text, &end);
#endif
	if ((end == text) || (*end != '\0')) abort();
	longdouble_store(destination, storage_size, value);
}

int64_t fb_LongDoubleToSigned(const void *source,
			      unsigned int storage_size)
{
	long double value = longdouble_load(source, storage_size);

	/* Avoid undefined C float-to-integer conversions at and beyond the limits. */
	if (isnan(value)) return 0;
	if (value >= 0x1p63L) return INT64_MAX;
	if (value <= -0x1p63L) return INT64_MIN;
	return (int64_t)value;
}

uint64_t fb_LongDoubleToUnsigned(const void *source,
				unsigned int storage_size)
{
	long double value = longdouble_load(source, storage_size);

	if (isnan(value) || (value <= 0.0L)) return 0;
	if (value >= 0x1p64L) return UINT64_MAX;
	return (uint64_t)value;
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
