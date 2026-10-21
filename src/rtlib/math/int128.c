/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: math/int128.c

    Purpose:

        Provide portable 128-bit integer operations for backends that use the
        compiler's two-word storage representation.

    Responsibilities:

        - operate on the compiler's low-word/high-word representation
        - implement arithmetic, bitwise operations, comparisons, and casts

    This file intentionally does NOT contain:

        - public language declarations
        - compiler parsing or instruction selection
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct FB_INT128_VALUE {
	uint64_t low;
	uint64_t high;
} FB_INT128_VALUE;

enum {
	FB_INT128_BOP_ADD = 1,
	FB_INT128_BOP_SUB,
	FB_INT128_BOP_MUL,
	FB_INT128_BOP_INTDIV,
	FB_INT128_BOP_MOD,
	FB_INT128_BOP_SHL,
	FB_INT128_BOP_SHR,
	FB_INT128_BOP_AND,
	FB_INT128_BOP_OR,
	FB_INT128_BOP_XOR,
	FB_INT128_BOP_EQV,
	FB_INT128_BOP_IMP,
	FB_INT128_BOP_EQ,
	FB_INT128_BOP_NE,
	FB_INT128_BOP_GT,
	FB_INT128_BOP_LT,
	FB_INT128_BOP_LE,
	FB_INT128_BOP_GE
};

enum {
	FB_INT128_UOP_NEG = 1,
	FB_INT128_UOP_NOT,
	FB_INT128_UOP_ABS,
	FB_INT128_UOP_SGN
};

static FB_INT128_VALUE int128_load(const void *address)
{
	FB_INT128_VALUE value;
	memcpy(&value, address, sizeof(value));
	return value;
}

static void int128_store(void *address, FB_INT128_VALUE value)
{
	memcpy(address, &value, sizeof(value));
}

static int int128_compare(FB_INT128_VALUE left, FB_INT128_VALUE right)
{
	if (left.high < right.high) return -1;
	if (left.high > right.high) return 1;
	if (left.low < right.low) return -1;
	if (left.low > right.low) return 1;
	return 0;
}

static FB_INT128_VALUE int128_negate(FB_INT128_VALUE value)
{
	FB_INT128_VALUE result;
	result.low = (uint64_t)0 - value.low;
	result.high = ~value.high + (result.low == 0);
	return result;
}

static FB_INT128_VALUE int128_subtract(FB_INT128_VALUE left, FB_INT128_VALUE right)
{
	FB_INT128_VALUE result;
	result.low = left.low - right.low;
	result.high = left.high - right.high - (left.low < right.low);
	return result;
}

static FB_INT128_VALUE int128_multiply(FB_INT128_VALUE left, FB_INT128_VALUE right)
{
	uint32_t left_word[4];
	uint32_t right_word[4];
	uint32_t result_word[4] = { 0, 0, 0, 0 };
	FB_INT128_VALUE result;
	int i;

	left_word[0] = (uint32_t)left.low;
	left_word[1] = (uint32_t)(left.low >> 32);
	left_word[2] = (uint32_t)left.high;
	left_word[3] = (uint32_t)(left.high >> 32);
	right_word[0] = (uint32_t)right.low;
	right_word[1] = (uint32_t)(right.low >> 32);
	right_word[2] = (uint32_t)right.high;
	right_word[3] = (uint32_t)(right.high >> 32);

	for (i = 0; i < 4; ++i) {
		uint64_t carry = 0;
		int j;
		for (j = 0; j < 4 - i; ++j) {
			uint64_t product = (uint64_t)left_word[i] * right_word[j];
			uint64_t sum = product + result_word[i + j] + carry;
			result_word[i + j] = (uint32_t)sum;
			carry = sum >> 32;
		}
	}

	result.low = (uint64_t)result_word[0] | ((uint64_t)result_word[1] << 32);
	result.high = (uint64_t)result_word[2] | ((uint64_t)result_word[3] << 32);
	return result;
}

static int int128_is_zero(FB_INT128_VALUE value)
{
	return value.low == 0 && value.high == 0;
}

static void int128_divide(FB_INT128_VALUE dividend, FB_INT128_VALUE divisor,
			  FB_INT128_VALUE *quotient, FB_INT128_VALUE *remainder)
{
	FB_INT128_VALUE q = { 0, 0 };
	FB_INT128_VALUE r = { 0, 0 };
	int bit;

	if (int128_is_zero(divisor)) abort();

	for (bit = 127; bit >= 0; --bit) {
		uint64_t carry = r.high >> 63;
		uint64_t input_bit;

		if (bit >= 64) {
			input_bit = (dividend.high >> (bit - 64)) & 1;
		} else {
			input_bit = (dividend.low >> bit) & 1;
		}

		r.high = (r.high << 1) | (r.low >> 63);
		r.low = (r.low << 1) | input_bit;
		if (carry || int128_compare(r, divisor) >= 0) {
			r = int128_subtract(r, divisor);
			if (bit >= 64) {
				q.high |= (uint64_t)1 << (bit - 64);
			} else {
				q.low |= (uint64_t)1 << bit;
			}
		}
	}

	*quotient = q;
	*remainder = r;
}

static int int128_signed_compare(FB_INT128_VALUE left, FB_INT128_VALUE right)
{
	int left_negative = (left.high >> 63) != 0;
	int right_negative = (right.high >> 63) != 0;
	if (left_negative != right_negative) return left_negative ? -1 : 1;
	return int128_compare(left, right);
}

void *fb_Int128Copy(void *destination, const void *source)
{
	memmove(destination, source, sizeof(FB_INT128_VALUE));
	return destination;
}

void *fb_Int128FromU64(void *destination, uint64_t value, int sign_extend)
{
	FB_INT128_VALUE result;
	result.low = value;
	result.high = (sign_extend && (value >> 63)) ? UINT64_MAX : 0;
	int128_store(destination, result);
	return destination;
}

void *fb_Int128FromDouble(void *destination, double source, int is_signed)
{
	const long double word_size = 18446744073709551616.0L;
	const long double signed_limit = 170141183460469231731687303715884105728.0L;
	const long double unsigned_limit = 340282366920938463463374607431768211456.0L;
	long double source_value = (long double)source;
	FB_INT128_VALUE result;
	int negative;
	long double magnitude;
	long double high_part;

	if (source_value < 0.0L) {
		negative = 1;
		magnitude = -source_value;
	} else {
		negative = 0;
		magnitude = source_value;
	}
	if (!(source_value == source_value)) abort();

	if (is_signed) {
		if ((negative && magnitude > signed_limit) ||
		    (!negative && magnitude >= signed_limit)) abort();
	} else if (negative || magnitude >= unsigned_limit) {
		abort();
	}

	/*
	 * The source is a double, so values at or above 2^64 already have an
	 * integral representation. Below 2^64, split the fraction explicitly and
	 * use the same nearest-even rule as nearbyint under the default rounding
	 * mode. Avoid converting a 128-bit value through a host integer.
	 */
	if (magnitude < word_size) {
		long double fraction;
		int round_up;

		result.low = (uint64_t)magnitude;
		fraction = magnitude - (long double)result.low;
		round_up = (fraction > 0.5L) ||
			((fraction == 0.5L) && ((result.low & 1) != 0));
		result.high = 0;
		if (round_up) {
			if (result.low == UINT64_MAX) {
				result.low = 0;
				result.high = 1;
			} else {
				result.low++;
			}
		}
	} else {
		high_part = magnitude / word_size;
		result.high = (uint64_t)high_part;
		result.low = (uint64_t)(magnitude - (long double)result.high * word_size);
	}
	if (negative) result = int128_negate(result);

	int128_store(destination, result);
	return destination;
}

uint64_t fb_Int128ToU64(const void *source)
{
	return int128_load(source).low;
}

int fb_Int128ToBool(const void *source)
{
	FB_INT128_VALUE value = int128_load(source);
	return value.low != 0 || value.high != 0;
}

double fb_Int128ToDouble(const void *source, int is_signed)
{
	const long double word_size = 18446744073709551616.0L;
	FB_INT128_VALUE value = int128_load(source);
	int negative = is_signed && ((value.high >> 63) != 0);
	long double magnitude;

	if (negative) value = int128_negate(value);
	magnitude = (long double)value.high * word_size + (long double)value.low;
	return (double)(negative ? -magnitude : magnitude);
}

void *fb_Int128Bop(void *destination, const void *left_address,
		   const void *right_address, int operation,
		   int is_signed, uint64_t shift_count)
{
	FB_INT128_VALUE left = int128_load(left_address);
	FB_INT128_VALUE right = { 0, 0 };
	FB_INT128_VALUE result = { 0, 0 };
	unsigned int shift = (unsigned int)(shift_count & 127);
	int comparison;

	if (operation != FB_INT128_BOP_SHL && operation != FB_INT128_BOP_SHR) {
		right = int128_load(right_address);
	}

	switch (operation) {
	case FB_INT128_BOP_ADD:
		result.low = left.low + right.low;
		result.high = left.high + right.high + (result.low < left.low);
		break;
	case FB_INT128_BOP_SUB:
		result = int128_subtract(left, right);
		break;
	case FB_INT128_BOP_MUL:
		result = int128_multiply(left, right);
		break;
	case FB_INT128_BOP_INTDIV:
	case FB_INT128_BOP_MOD: {
		FB_INT128_VALUE dividend = left;
		FB_INT128_VALUE divisor = right;
		FB_INT128_VALUE quotient;
		FB_INT128_VALUE remainder;
		int left_negative = is_signed && ((left.high >> 63) != 0);
		int right_negative = is_signed && ((right.high >> 63) != 0);
		if (left_negative) dividend = int128_negate(dividend);
		if (right_negative) divisor = int128_negate(divisor);
		int128_divide(dividend, divisor, &quotient, &remainder);
		if (operation == FB_INT128_BOP_INTDIV) {
			result = quotient;
			if (left_negative != right_negative) result = int128_negate(result);
		} else {
			result = remainder;
			if (left_negative) result = int128_negate(result);
		}
		break;
	}
	case FB_INT128_BOP_SHL:
		if (shift == 0) {
			result = left;
		} else if (shift < 64) {
			result.low = left.low << shift;
			result.high = (left.high << shift) | (left.low >> (64 - shift));
		} else {
			result.high = left.low << (shift - 64);
		}
		break;
	case FB_INT128_BOP_SHR:
		if (shift == 0) {
			result = left;
		} else if (shift < 64) {
			result.low = (left.low >> shift) | (left.high << (64 - shift));
			result.high = left.high >> shift;
		} else {
			result.low = left.high >> (shift - 64);
			result.high = 0;
		}
		if (is_signed && (left.high >> 63)) {
			if (shift > 64) {
				result.high = UINT64_MAX;
				result.low |= UINT64_MAX << (128 - shift);
			} else if (shift == 64) {
				result.high = UINT64_MAX;
			} else if (shift != 0) {
				result.high |= UINT64_MAX << (64 - shift);
			}
		}
		break;
	case FB_INT128_BOP_AND:
		result.low = left.low & right.low;
		result.high = left.high & right.high;
		break;
	case FB_INT128_BOP_OR:
		result.low = left.low | right.low;
		result.high = left.high | right.high;
		break;
	case FB_INT128_BOP_XOR:
		result.low = left.low ^ right.low;
		result.high = left.high ^ right.high;
		break;
	case FB_INT128_BOP_EQV:
		result.low = ~(left.low ^ right.low);
		result.high = ~(left.high ^ right.high);
		break;
	case FB_INT128_BOP_IMP:
		result.low = ~left.low | right.low;
		result.high = ~left.high | right.high;
		break;
	case FB_INT128_BOP_EQ:
	case FB_INT128_BOP_NE:
	case FB_INT128_BOP_GT:
	case FB_INT128_BOP_LT:
	case FB_INT128_BOP_LE:
	case FB_INT128_BOP_GE:
		comparison = is_signed ? int128_signed_compare(left, right) : int128_compare(left, right);
		{
			intptr_t value = 0;
			switch (operation) {
			case FB_INT128_BOP_EQ: value = comparison == 0; break;
			case FB_INT128_BOP_NE: value = comparison != 0; break;
			case FB_INT128_BOP_GT: value = comparison > 0; break;
			case FB_INT128_BOP_LT: value = comparison < 0; break;
			case FB_INT128_BOP_LE: value = comparison <= 0; break;
			case FB_INT128_BOP_GE: value = comparison >= 0; break;
			}
			memcpy(destination, &value, sizeof(value));
		}
		return destination;
	default:
		abort();
	}

	int128_store(destination, result);
	return destination;
}

void *fb_Int128Uop(void *destination, const void *source,
		   int operation, int is_signed)
{
	FB_INT128_VALUE value = int128_load(source);
	FB_INT128_VALUE result = { 0, 0 };
	int comparison;

	switch (operation) {
	case FB_INT128_UOP_NEG:
		result = int128_negate(value);
		break;
	case FB_INT128_UOP_NOT:
		result.low = ~value.low;
		result.high = ~value.high;
		break;
	case FB_INT128_UOP_ABS:
		result = (is_signed && ((value.high >> 63) != 0)) ? int128_negate(value) : value;
		break;
	case FB_INT128_UOP_SGN:
		comparison = int128_is_zero(value) ? 0 : (((value.high >> 63) && is_signed) ? -1 : 1);
		result.low = (uint64_t)(int64_t)comparison;
		result.high = comparison < 0 ? UINT64_MAX : 0;
		break;
	default:
		abort();
	}

	int128_store(destination, result);
	return destination;
}

/* end of math/int128.c */
