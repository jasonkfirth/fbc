/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/strw_bin_lng.c

    Purpose:

        Format 64-bit wide binary values without an AROS m68k GCC defect.

    Responsibilities:

        - count significant bits through explicit high and low words
        - render wide binary text from an unsigned 64-bit value
        - preserve the generic WBIN runtime contract

    This file intentionally does NOT contain:

        - generic m68k integer policy
        - an AROS 68000 instruction baseline
        - formatting for non-binary radices
*/

#include "../../fb.h"

typedef union FB_AROS_M68K_WBIN_WORDS {
	unsigned long long number;
	struct {
		uint32_t high;
		uint32_t low;
	} word;
} FB_AROS_M68K_WBIN_WORDS;

static void fb_hArosWbinShiftRightOne( FB_AROS_M68K_WBIN_WORDS *value )
{
	value->word.low = (value->word.low >> 1) |
		(value->word.high << 31);
	value->word.high >>= 1;
}

FBCALL FB_WCHAR *fb_WstrBinEx_l( unsigned long long num, int digits )
{
	FB_AROS_M68K_WBIN_WORDS remaining;
	FB_WCHAR *result;
	int index;

	if( digits <= 0 ) {
		digits = 0;
		remaining.number = num;
		while( (remaining.word.high != 0) ||
		       (remaining.word.low != 0) ) {
			++digits;
			fb_hArosWbinShiftRightOne( &remaining );
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
		result[index] = _LC( '0' ) + (remaining.word.low & 1);
		fb_hArosWbinShiftRightOne( &remaining );
	}

	result[digits] = _LC( '\0' );
	return result;
}

FBCALL FB_WCHAR *fb_WstrBin_l( unsigned long long num )
{
	return fb_WstrBinEx_l( num, 0 );
}

/* end of aros/m68k/strw_bin_lng.c */
