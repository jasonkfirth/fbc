/*
    FreeBASIC Runtime Library
    -------------------------

    File: dos/str_convfrom.c

    Purpose:

        Convert strings to floating point values on the DOS target.

    Responsibilities:

        - implement VAL() and the internal string-to-double helper
        - preserve FreeBASIC radix prefixes handled by the shared runtime
        - avoid depending on the DOS C library strtod() implementation

    This file intentionally does NOT contain:

        - integer conversion helpers
        - formatting code
        - locale-specific decimal separator handling

    DOS note:

        The DOS-hosted compiler uses VAL() while parsing floating point
        literals.  Some DJGPP/runtime combinations return an invalid NaN
        for ordinary decimal inputs through strtod(), which makes the
        compiler emit bad constants.  The parser below keeps DOS numeric
        literal handling deterministic and independent from that C library
        behavior.
*/

#include "../fb.h"
#include <math.h>


/* ------------------------------------------------------------------------- */
/* Decimal parser                                                            */
/* ------------------------------------------------------------------------- */

static int fb_hDosIsDigit( int c )
{
	return (c >= '0') && (c <= '9');
}

static int fb_hDosIsSpace( int c )
{
	return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

static void fb_hDosResetFpuStack( void )
{
	unsigned short control_word;

	/*
		DJGPP code uses the x87 FPU for ordinary double arithmetic.

		The DOS-hosted compiler can call VAL() after earlier compiler
		work has left temporaries on the x87 stack.  A full x87 stack
		makes otherwise normal floating point operations produce NaNs.
		VAL() is a normal function call boundary, so callers cannot
		depend on live x87 registers surviving across it.  Preserve the
		control word, but clear the register stack before parsing.
	*/
	__asm__ __volatile__(
		"fnstcw %0\n\t"
		"fninit\n\t"
		"fldcw %0"
		: "=m" (control_word)
		:
		: "memory"
	);
}

static double fb_hDosParseDecimal( const char *p, ssize_t len )
{
	const char *end = p + len;
	double value = 0.0;
	double fraction = 0.1;
	int sign = 1;
	int exponent_sign = 1;
	int exponent = 0;
	int have_digit = 0;

	while( (p < end) && fb_hDosIsSpace( (unsigned char)*p ) )
		++p;

	if( p >= end )
		return 0.0;

	if( *p == '-' ) {
		sign = -1;
		++p;
	} else if( *p == '+' ) {
		++p;
	}

	while( (p < end) && fb_hDosIsDigit( (unsigned char)*p ) ) {
		value = (value * 10.0) + (double)(*p - '0');
		have_digit = 1;
		++p;
	}

	if( (p < end) && (*p == '.') ) {
		++p;

		while( (p < end) && fb_hDosIsDigit( (unsigned char)*p ) ) {
			value += (double)(*p - '0') * fraction;
			fraction *= 0.1;
			have_digit = 1;
			++p;
		}
	}

	if( !have_digit )
		return 0.0;

	if( (p < end) && ((*p == 'e') || (*p == 'E') || (*p == 'd') || (*p == 'D')) ) {
		const char *exp_start = p;
		int have_exponent_digit = 0;

		++p;

		if( p < end ) {
			if( *p == '-' ) {
				exponent_sign = -1;
				++p;
			} else if( *p == '+' ) {
				++p;
			}
		}

		while( (p < end) && fb_hDosIsDigit( (unsigned char)*p ) ) {
			if( exponent < 10000 )
				exponent = (exponent * 10) + (*p - '0');

			have_exponent_digit = 1;
			++p;
		}

		if( !have_exponent_digit ) {
			p = exp_start;
			exponent = 0;
			exponent_sign = 1;
		}
	}

	if( exponent != 0 )
		value *= pow( 10.0, (double)(exponent * exponent_sign) );

	if( sign < 0 )
		value = -value;

	return value;
}


/* ------------------------------------------------------------------------- */
/* Public conversion entry points                                            */
/* ------------------------------------------------------------------------- */

FBCALL double fb_hStr2Double( char *src, ssize_t len )
{
	char *p;
	int radix, skip;

	fb_hDosResetFpuStack();

	p = fb_hStrSkipChar( src, len, 32 );

	len -= (ssize_t)(p - src);
	if( len < 1 )
		return 0.0;

	if( len >= 2 ) {
		if( p[0] == '&' ) {
			skip = 2;
			switch( p[1] ) {
			case 'h':
			case 'H':
				radix = 16;
				break;
			case 'o':
			case 'O':
				radix = 8;
				break;
			case 'b':
			case 'B':
				radix = 2;
				break;

			default:
				radix = 8;
				skip = 1;
				break;
			}

			return fb_hStrRadix2Longint( p + skip, len - skip, radix );

		} else if( p[0] == '0' ) {
			if( (p[1] == 'x') || (p[1] == 'X') )
				return 0.0;
		}
	}

	return fb_hDosParseDecimal( p, len );
}

FBCALL double fb_VAL( FBSTRING *str )
{
	double val;

	if( str == NULL )
		return 0.0;

	if( (str->data == NULL) || (FB_STRSIZE( str ) == 0) )
		val = 0.0;
	else
		val = fb_hStr2Double( str->data, FB_STRSIZE( str ) );

	fb_hStrDelTemp( str );

	return val;
}

/* end of dos/str_convfrom.c */
