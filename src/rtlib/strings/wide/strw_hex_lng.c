/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_hex_lng.c
 * Purpose: hexw$ routine for long long's.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static FB_WCHAR hex_table[16] = {FB_WCHAR_LITERAL('0'),FB_WCHAR_LITERAL('1'),FB_WCHAR_LITERAL('2'),FB_WCHAR_LITERAL('3'),
								 FB_WCHAR_LITERAL('4'),FB_WCHAR_LITERAL('5'),FB_WCHAR_LITERAL('6'),FB_WCHAR_LITERAL('7'),
								 FB_WCHAR_LITERAL('8'),FB_WCHAR_LITERAL('9'),FB_WCHAR_LITERAL('A'),FB_WCHAR_LITERAL('B'),
								 FB_WCHAR_LITERAL('C'),FB_WCHAR_LITERAL('D'),FB_WCHAR_LITERAL('E'),FB_WCHAR_LITERAL('F')};

FBCALL FB_WCHAR *fb_WstrHexEx_l ( unsigned long long num, int digits )
{
	FB_WCHAR *s;
	int i;
	unsigned long long num2;

	if( digits <= 0 ) {
		/* Only use the minimum amount of digits needed; need to count
		   the important 4-bit (base 16) chunks in the number.
		   And if it's zero, use 1 digit for 1 zero. */
		digits = 0;
		num2 = num;
		while( num2 ) {
			digits += 1;
			num2 >>= 4;
		}
		if( digits == 0 )
			digits = 1;
	}

	s = fb_wstr_AllocTemp( digits );
	if( s == NULL )
		return NULL;

	i = digits - 1;
	while( i >= 0 ) {
		s[i] = hex_table[num & 0xF];
		num >>= 4;
		i -= 1;
	}

	s[digits] = FB_WCHAR_LITERAL('\0');
	return s;
}

FBCALL FB_WCHAR *fb_WstrHex_l ( unsigned long long num )
{
	return fb_WstrHexEx_l( num, 0 );
}

/* end of strings/wide/strw_hex_lng.c */
