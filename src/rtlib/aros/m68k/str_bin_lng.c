/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/str_bin_lng.c

    Purpose:

        Format 64-bit binary values without the AROS m68k GCC shift-test bug.

    Responsibilities:

        - count significant bits through explicit high and low words
        - render the requested binary digits from an unsigned 64-bit value
        - preserve the generic BIN runtime contract

    This file intentionally does NOT contain:

        - generic m68k integer policy
        - an AROS 68000 instruction baseline
        - formatting for non-binary radices

    Toolchain behavior:

        AROS m68k GCC 6.5 can test only the low word after a shifted uint64_t
        loop condition.  A value with only high bits set then receives too few
        default digits.  Explicit word tests avoid that optimizer defect.
*/

#include "../../fb.h"

typedef union FB_AROS_M68K_UINT64_WORDS {
	unsigned long long number;
	struct {
		uint32_t high;
		uint32_t low;
	} word;
} FB_AROS_M68K_UINT64_WORDS;

static void fb_hArosShiftRightOne( FB_AROS_M68K_UINT64_WORDS *value )
{
	value->word.low = (value->word.low >> 1) |
		(value->word.high << 31);
	value->word.high >>= 1;
}

FBCALL FBSTRING *fb_BINEx_l( unsigned long long num, int digits )
{
	FB_AROS_M68K_UINT64_WORDS remaining;
	FBSTRING *result;
	int index;

	if( digits <= 0 ) {
		digits = 0;
		remaining.number = num;
		while( (remaining.word.high != 0) ||
		       (remaining.word.low != 0) ) {
			++digits;
			fb_hArosShiftRightOne( &remaining );
		}
		if( digits == 0 ) {
			digits = 1;
		}
	}

	result = fb_hStrAllocTemp( NULL, digits );
	if( result == NULL ) {
		return &__fb_ctx.null_desc;
	}

	remaining.number = num;
	for( index = digits - 1; index >= 0; --index ) {
		result->data[index] = '0' + (remaining.word.low & 1);
		fb_hArosShiftRightOne( &remaining );
	}

	result->data[digits] = '\0';
	return result;
}

FBCALL FBSTRING *fb_BIN_l( unsigned long long num )
{
	return fb_BINEx_l( num, 0 );
}

/* end of aros/m68k/str_bin_lng.c */
