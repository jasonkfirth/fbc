/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/strw_oct_lng.c

    Purpose:

        Format 64-bit wide octal values without an AROS m68k GCC defect.

    Responsibilities:

        - count significant octal groups through explicit high and low words
        - render wide octal text from an unsigned 64-bit value
        - preserve the generic WOCT runtime contract

    This file intentionally does NOT contain:

        - generic m68k integer policy
        - an AROS 68000 instruction baseline
        - formatting for non-octal radices
*/

#include "../../fb.h"

typedef union FB_AROS_M68K_WOCT_WORDS {
	unsigned long long number;
	struct {
		uint32_t high;
		uint32_t low;
	} word;
} FB_AROS_M68K_WOCT_WORDS;

static void fb_hArosWoctShiftRightThree( FB_AROS_M68K_WOCT_WORDS *value )
{
	value->word.low = (value->word.low >> 3) |
		(value->word.high << 29);
	value->word.high >>= 3;
}

FBCALL FB_WCHAR *fb_WstrOctEx_l( unsigned long long num, int digits )
{
	FB_AROS_M68K_WOCT_WORDS remaining;
	FB_WCHAR *result;
	int index;

	if( digits <= 0 ) {
		digits = 0;
		remaining.number = num;
		while( (remaining.word.high != 0) ||
		       (remaining.word.low != 0) ) {
			++digits;
			fb_hArosWoctShiftRightThree( &remaining );
		}
		if( digits == 0 ) {
			digits = 1;
		}
	}

	result = fb_wstr_AllocTemp( digits );
	if( result == NULL ) {
		return NULL;
	}

	remaining.number = num;
	for( index = digits - 1; index >= 0; --index ) {
		result[index] = _LC( '0' ) + (remaining.word.low & 7);
		fb_hArosWoctShiftRightThree( &remaining );
	}

	result[digits] = _LC( '\0' );
	return result;
}

FBCALL FB_WCHAR *fb_WstrOct_l( unsigned long long num )
{
	return fb_WstrOctEx_l( num, 0 );
}

/* end of aros/m68k/strw_oct_lng.c */
