/*
 * Project: FreeBASIC compiler regression tests
 * File: llvm-small-struct-return-helper.c
 *
 * Purpose:
 *     Provide native x86-64 Linux aggregate return calls for the LLVM test.
 *
 * Responsibilities:
 *     Return C aggregates to BASIC and check aggregates returned by BASIC.
 *
 * This file intentionally does NOT contain:
 *     LLVM IR or a substitute for the C compiler's ABI lowering.
 */

#include <stdarg.h>
#include <stdint.h>

struct pair {
	int32_t first;
	int32_t second;
};

struct floats {
	float items[3];
};

struct mixed {
	double number;
	int32_t marker;
};

struct __attribute__((packed)) packed_result {
	uint8_t tag;
	int32_t payload;
};

struct __attribute__((packed)) nested_result {
	uint8_t prefix[3];
	struct packed_result inner;
};

struct combined {
	float number;
	int32_t marker;
};

union overlay {
	float number;
	uint32_t bits;
};

extern struct pair llvm_return_pair(int32_t first);
extern struct floats llvm_return_floats(void);
extern struct mixed llvm_return_mixed(void);
extern struct packed_result llvm_return_packed(int32_t payload);
extern struct nested_result llvm_return_nested(void);
extern struct combined llvm_return_combined(void);
extern union overlay llvm_return_overlay(void);

static int32_t packed_calls;

struct pair c_return_pair(int32_t first)
{
	return (struct pair){first, -123456789};
}

struct floats c_return_floats(void)
{
	return (struct floats){{1.25f, -2.5f, 3.75f}};
}

struct mixed c_return_mixed(void)
{
	return (struct mixed){-14.75, 12345};
}

struct packed_result c_return_packed(int32_t payload)
{
	++packed_calls;
	return (struct packed_result){0xA5, payload};
}

struct packed_result c_return_variadic(int32_t base, ...)
{
	va_list args;
	int32_t payload;

	va_start(args, base);
	payload = va_arg(args, int32_t);
	va_end(args);
	return (struct packed_result){0xBC, base + payload};
}

struct nested_result c_return_nested(void)
{
	return (struct nested_result){{21, 22, 23}, {24, -25}};
}

struct combined c_return_combined(void)
{
	return (struct combined){11.5f, -6789};
}

union overlay c_return_overlay(void)
{
	return (union overlay){.bits = UINT32_C(0xDEADBEEF)};
}

int32_t c_check_llvm_results(void)
{
	struct pair pair = llvm_return_pair(-456);
	struct floats floats = llvm_return_floats();
	struct mixed mixed = llvm_return_mixed();
	struct packed_result packed = llvm_return_packed(-271828);
	struct nested_result nested = llvm_return_nested();
	struct combined combined = llvm_return_combined();
	union overlay overlay = llvm_return_overlay();

	if (pair.first != -456 || pair.second != 789)
		return 1;
	if (floats.items[0] != 3.5f || floats.items[1] != -4.75f || floats.items[2] != 6.25f)
		return 2;
	if (mixed.number != 29.125 || mixed.marker != -8128)
		return 3;
	if (packed.tag != 0x5A || packed.payload != -271828 || packed_calls != 3)
		return 4;
	if (combined.number != -9.25f || combined.marker != 4567)
		return 5;
	if (overlay.bits != UINT32_C(0x12345678))
		return 6;
	if (nested.prefix[0] != 31 || nested.prefix[1] != 32 || nested.prefix[2] != 33 ||
	    nested.inner.tag != 34 || nested.inner.payload != -35)
		return 7;
	return 0;
}

/* end of llvm-small-struct-return-helper.c */
