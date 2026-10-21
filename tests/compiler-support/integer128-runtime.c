/*
    Project: FreeBASIC compiler backend regression tests
    ---------------------------------------------------

    File: integer128-runtime.c

    Purpose:

        Exercise the portable two-word 128-bit runtime used by the 32-bit
        assembly and C compiler backends.

    Responsibilities:

        - check carry, multiplication, division and remainder
        - check logical and arithmetic shifts and signed comparisons
        - check integer and floating-point conversion helpers
        - provide the small termination entry point used by generated C

    This file intentionally does NOT contain:

        - compiler front-end or backend code
        - assumptions about the host's native __int128 support
*/

#include <stdint.h>
#include <stdlib.h>

typedef struct FB_INT128_TEST_VALUE {
	uint64_t low;
	uint64_t high;
} FB_INT128_TEST_VALUE;

enum {
	FB_INT128_TEST_ADD = 1,
	FB_INT128_TEST_SUB,
	FB_INT128_TEST_MUL,
	FB_INT128_TEST_INTDIV,
	FB_INT128_TEST_MOD,
	FB_INT128_TEST_SHL,
	FB_INT128_TEST_SHR,
	FB_INT128_TEST_AND,
	FB_INT128_TEST_OR,
	FB_INT128_TEST_XOR,
	FB_INT128_TEST_EQV,
	FB_INT128_TEST_IMP,
	FB_INT128_TEST_EQ,
	FB_INT128_TEST_NE,
	FB_INT128_TEST_GT,
	FB_INT128_TEST_LT,
	FB_INT128_TEST_LE,
	FB_INT128_TEST_GE
};

enum {
	FB_INT128_TEST_NEG = 1,
	FB_INT128_TEST_NOT,
	FB_INT128_TEST_ABS,
	FB_INT128_TEST_SGN
};

extern void *fb_Int128FromU64(void *destination, uint64_t value, int sign_extend);
extern void *fb_Int128FromDouble(void *destination, double value, int is_signed);
extern uint64_t fb_Int128ToU64(const void *source);
extern double fb_Int128ToDouble(const void *source, int is_signed);
extern void *fb_Int128Bop(void *destination, const void *left, const void *right,
			  int operation, int is_signed, uint64_t shift_count);
extern void *fb_Int128Uop(void *destination, const void *source,
			  int operation, int is_signed);

void fb_End(int exit_code)
{
	exit(exit_code);
}

static int int128_equals(const FB_INT128_TEST_VALUE *value,
			 uint64_t low, uint64_t high)
{
	return value->low == low && value->high == high;
}

int main(void)
{
	FB_INT128_TEST_VALUE left;
	FB_INT128_TEST_VALUE right;
	FB_INT128_TEST_VALUE result;
	intptr_t comparison = 0;

	fb_Int128FromU64(&left, UINT64_MAX, 0);
	fb_Int128FromU64(&right, 1, 0);
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_ADD, 0, 0);
	if (!int128_equals(&result, 0, 1)) return 1;

	left.low = 0;
	left.high = 1;
	fb_Int128FromU64(&right, 3, 0);
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_MUL, 0, 0);
	if (!int128_equals(&result, 0, 3)) return 2;

	fb_Int128FromU64(&left, 0, 0);
	left.high = 1;
	fb_Int128FromU64(&right, 5, 0);
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_INTDIV, 0, 0);
	if (!int128_equals(&result, UINT64_C(0x3333333333333333), 0)) return 3;
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_MOD, 0, 0);
	if (!int128_equals(&result, 1, 0)) return 4;

	fb_Int128FromU64(&left, (uint64_t)-100, 1);
	fb_Int128FromU64(&right, 7, 0);
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_INTDIV, 1, 0);
	if (!int128_equals(&result, (uint64_t)-14, UINT64_MAX)) return 5;
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_MOD, 1, 0);
	if (!int128_equals(&result, (uint64_t)-2, UINT64_MAX)) return 6;

	left.low = 0;
	left.high = 1;
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_SHR, 0, 64);
	if (!int128_equals(&result, 1, 0)) return 7;
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_SHL, 0, 63);
	if (!int128_equals(&result, 0, UINT64_C(0x8000000000000000))) return 8;

	fb_Int128FromU64(&left, (uint64_t)-8, 1);
	fb_Int128Bop(&result, &left, &right, FB_INT128_TEST_SHR, 1, 2);
	if (!int128_equals(&result, (uint64_t)-2, UINT64_MAX)) return 9;

	left.high = UINT64_C(0x8000000000000000);
	left.low = 0;
	right.high = UINT64_C(0x7fffffffffffffff);
	right.low = UINT64_MAX;
	fb_Int128Bop(&comparison, &left, &right, FB_INT128_TEST_GT, 0, 0);
	if (comparison != 1) return 10;
	fb_Int128Bop(&comparison, &left, &right, FB_INT128_TEST_GT, 1, 0);
	if (comparison != 0) return 11;

	fb_Int128FromDouble(&result, 12.75, 1);
	if (!int128_equals(&result, 13, 0)) return 12;
	if (fb_Int128ToU64(&result) != 13) return 13;
	if (fb_Int128ToDouble(&result, 1) != 13.0) return 14;

	return 0;
}

/* end of integer128-runtime.c */
