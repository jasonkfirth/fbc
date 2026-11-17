/*
    Project: FreeBASIC Runtime Library
    File: console/io_printusg.c
    Purpose: Implement PRINT USING for byte, wide, and UTF-8 text and numbers.
    Responsibilities: Preserve format state, field widths, and output ownership.
    This file intentionally does NOT contain: device creation or source parsing.
*/

/* print using function */

#include "fb.h"
#include <math.h>
#include <string.h>

typedef struct {
	ssize_t   chars;
	char     *ptr;
	FBSTRING  fmtstr;
	int       utf8;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	/* Name the existing 64-bit tail padding without changing the layout. */
	unsigned char tail_padding[4];
#endif
} FB_PRINTUSGCTX;

#define fb_PRINTUSGCTX_Destructor NULL

#define BUFFERLEN 2048
#define MIN_EXPDIGS 3
#define MAX_EXPDIGS 5
#define MAX_DIGS (BUFFERLEN                                 \
                   - 2                 /* '%' char(s)   */  \
                   - 1                 /* +/- sign      */  \
                   - 1                 /* dollar sign   */  \
                   - 1                 /* decimal point */  \
                   - MAX_EXPDIGS       /* exp digits    */  \
                   - (MIN_EXPDIGS - 1) /* stray carets  */  \
                 )


#define CHAR_ZERO        '0'
#define CHAR_DOT         '.'
#define CHAR_COMMA       ','
#define CHAR_TOOBIG      '%'
#define CHAR_PLUS        '+'
#define CHAR_MINUS       '-'
#define CHAR_STAR        '*'
#define CHAR_DOLLAR      '$'
#define CHAR_SPACE       ' '
#define CHAR_WTF         '!'
#define CHAR_EXP_SINGLE  'E'
#if 0
#define CHAR_EXP_DOUBLE  'D'
#endif

#define SNG_AUTODIGS 7
#define DBL_MAXDIGS 16

#define CHARS_NAN  ('#' << 24 | 'N' << 16 | 'A' << 8 | 'N')
#define CHARS_INF  ('#' << 24 | 'I' << 16 | 'N' << 8 | 'F')
#define CHARS_IND  ('#' << 24 | 'I' << 16 | 'N' << 8 | 'D')
#define CHARS_TRUNC ('$' << 24 | '0' << 16 | '0' << 8 | '0') /* QB glitch: truncation "rounds up" the text chars */

#define CHARS_TRUE  ('t' << 24 | 'r' << 16 | 'u' << 8 | 'e')
#define CHARS_FALSE (((uint64_t)'f') << 32 | 'a' << 24 | 'l' << 16 | 's' << 8 | 'e')

#define ADD_CHAR( c )              \
    do {                           \
        DBG_ASSERT( p >= buffer ); \
        if( p >= buffer )          \
            *(p--) = (char)(c);    \
        else if( p == buffer )     \
            *p = CHAR_WTF;         \
    } while (0)


/*-------------------------------------------------------------*/
/* Checks for Infinity/NaN                                     *
 * (assumes IEEE-754 floating-point format)                    *
 * TODO: use a proper implementation: most/all platforms       *
 * have specific functions built-in for this                   */

static unsigned long long hDoubleToLongBits(double d)
{
	union{ double d; unsigned long long ll; } dtoll;
	dtoll.d = d;
	return dtoll.ll;
}

static int hIsNeg(double d)
{
	return (hDoubleToLongBits(d) & 0x8000000000000000ull) != 0ull;
}

static int hIsZero(double d)
{
	return (hDoubleToLongBits(d) & 0x7fffffffffffffffull) == 0ull;
}

static int hIsFinite(double d)
{
	return (hDoubleToLongBits(d) & 0x7ff0000000000000ull) < 0x7ff0000000000000ull;
}

static int hIsInf(double d)
{
	return (hDoubleToLongBits(d) & 0x7fffffffffffffffull) == 0x7ff0000000000000ull;
}

static int hIsInd(double d)
{
	return hDoubleToLongBits(d) == 0xfff8000000000000ull;
}

static int hIsNan(double d)
{
	return !(hIsFinite(d) || hIsInf(d) || hIsInd(d));
}



/*-------------------------------------------------------------*/

#define VAL_ISNEG 0x1
#define VAL_ISINF 0x2
#define VAL_ISIND 0x4
#define VAL_ISNAN 0x8

#define VAL_ISFLOAT 0x10
#define VAL_ISSNG 0x20

#define VAL_ISBOOL 0x40


/* PRINT USING keeps its existing byte format grammar. Unicode arguments
   select scalar widths and UTF-8 output; numeric formatting is shared. */
static void hPrintText( int fnum, const char *text, ssize_t length )
{
	FB_PRINTUSGCTX *ctx = FB_TLSGETCTX( PRINTUSG );
	if( ctx->utf8 ) {
		/* FBSTRING keeps its mutable ABI field, while this borrowed view is read-only. */
		FBSTRING view = { NULL, length, length };
		memcpy((void *)&view.data, (const void *)&text, sizeof(view.data));
		fb_UStrPrint( fnum, &view, 0 );
	} else fb_PrintFixString( fnum, text, 0 );
}

static void hPrintField( int fnum, FBSTRING_ACCESS *source, ssize_t columns, int unicode )
{
	ssize_t length = source && source->data ? FB_STRSIZE(source) : 0;
	ssize_t count;
	ssize_t bytes;
	ssize_t padding;
	ssize_t i;
	FBSTRING_ACCESS *result;
	if( source && source->data ) {
		count = unicode ? fb_hUtf8Count(source->data, length) : length;
		bytes = unicode ? fb_hUtf8Offset(source->data, length, columns) :
			(length < columns ? length : columns);
	} else {
		count = 0;
		bytes = 0;
	}
	padding = count < columns ? columns - count : 0;
	FB_STRLOCK();
	result = NULL;
	if( (bytes >= 0) && (padding >= 0) &&
	    (padding <= FB_USTRING_MAX_BYTES) &&
	    (bytes <= FB_USTRING_MAX_BYTES - padding) )
		result = fb_hUStrAlloc_NoLock(bytes + padding);
	if( result != NULL && bytes + padding > 0 ) {
		if( bytes ) memcpy(result->data, source->data, (size_t)bytes);
		/* Fixed fields retain BASIC's NUL-to-space convention. */
		for( i = 0; i < bytes; ++i ) if( result->data[i] == 0 ) result->data[i] = ' ';
		memset(result->data + bytes, ' ', (size_t)padding);
	}
	FB_STRUNLOCK();
	if( result != NULL ) {
		hPrintText(fnum, result->data ? result->data : "", FB_STRSIZE(result));
		fb_hStrDelTemp(result);
	}
}

static int fb_PrintUsingFmtStr( int fnum );

FBCALL int fb_PrintUsingInit( FBSTRING_ACCESS *fmtstr )
{
    FB_PRINTUSGCTX *ctx;

    FB_LOCK();

    ctx = FB_TLSGETCTX( PRINTUSG );

	fb_StrAssign( (void *)&ctx->fmtstr, -1, fmtstr, -1, 0 );
	ctx->ptr = ctx->fmtstr.data;
	ctx->chars = FB_STRSIZE( &ctx->fmtstr );
	ctx->utf8 = FALSE;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_PrintUsingInitUstr( FBSTRING_ACCESS *format )
{
	int result = fb_PrintUsingInit(format);
	FB_PRINTUSGCTX *ctx = FB_TLSGETCTX( PRINTUSG );
	ctx->utf8 = TRUE;
	return result;
}

FBCALL int fb_PrintUsingEnd( int fnum )
{
	FB_PRINTUSGCTX *ctx;

	fb_PrintUsingFmtStr( fnum );

	FB_LOCK();

	ctx = FB_TLSGETCTX( PRINTUSG );

	fb_StrDelete( &ctx->fmtstr );
	ctx->ptr = 0;
	ctx->chars = 0;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

static unsigned long long hPow10_ULL( int n )
{

	DBG_ASSERT( n >= 0 && n <= 19 );

	unsigned long long ret = 1, a = 10;
	while( n > 0 )
	{
		if( n & 1 ) ret *= a;
		a *= a;
		n >>= 1;
	}

	return ret;
}

static int hLog10_ULL( unsigned long long a )
{
	unsigned int ret = 0;
	unsigned int a32;
	unsigned long long a64;

	a64 = a;
	while( a64 >= (unsigned int)1.E+8 )
	{
		a64 /= (unsigned int)1.E+8;
		ret += 8u;
	}
	/* The loop leaves a64 below 100,000,000, which fits in a32. */
	a32 = (unsigned int)a64;
	if( a32 >= (unsigned int)1.E+4 ) ret += 4u; else a32 *= (unsigned int)1.E+4;
	if( a32 >= (unsigned int)1.E+6 ) ret += 2u; else a32 *= (unsigned int)1.E+2;
	if( a32 >= (unsigned int)1.E+7 ) ret += 1u;

	if( a == 0 )
		DBG_ASSERT( ret == 0u );
	else
		DBG_ASSERT( hPow10_ULL( (int)ret ) <= a && hPow10_ULL( (int)ret ) > a / 10 );

	return (int)ret;
}

static int hNumDigits( unsigned long long a )
{
	 return hLog10_ULL( a ) + 1;
}

/* Compare against a negative magnitude without signed negation. */
static uint64_t hInt64Magnitude(int64_t value)
{
	return (value < 0) ? UINT64_C(0) - (uint64_t)value : (uint64_t)value;
}

static int hInt64GreaterThanNegativeMagnitude(int64_t value,
	uint64_t magnitude)
{
	if (magnitude == 0u)
		return value > 0;
	if (value >= 0)
		return TRUE;
	return hInt64Magnitude(value) < magnitude;
}

static int hInt64LessThanNegativeMagnitude(int64_t value,
	uint64_t magnitude)
{
	if (magnitude == 0u)
		return value < 0;
	if (value >= 0)
		return FALSE;
	return hInt64Magnitude(value) > magnitude;
}

static int64_t hIncrementDigitCount(int64_t digits)
{
	if (digits == -1)
		return 0;
	if ((digits < 0) || ((uint64_t)digits >= (uint64_t)INT64_MAX))
		return digits;
	return (int64_t)((uint64_t)digits + UINT64_C(1));
}

static unsigned long long hDivPow10_ULL( unsigned long long a, int n )
{
	unsigned long long b, ret;

	DBG_ASSERT( n >= 0 );

	if( n > 19 ) return 0;

	b = hPow10_ULL( n );
	ret = a / b;

	if( (a % b) >= (b + 1) / 2 )
		ret += 1; /* round up */

	return ret;
}

static int fb_PrintUsingFmtStr( int fnum )
{
	FB_PRINTUSGCTX *ctx;
	char buffer[BUFFERLEN+1];
	int c, nc, nnc, len, doexit;

	ctx = FB_TLSGETCTX( PRINTUSG );

	len = 0;
	if( ctx->ptr == NULL )
		ctx->chars = 0;

	while( ctx->chars > 0 )
	{
		/* Keep a whole UTF-8 scalar in each flush. The encoded file hook
		   receives independent chunks, so it cannot join split sequences. */
		if( len > BUFFERLEN - 4 ) {
			buffer[len] = 0;
			hPrintText(fnum, buffer, len);
			len = 0;
		}
		c = FB_CHAR_TO_INT( *ctx->ptr );
		nc = ( ctx->chars > 1? FB_CHAR_TO_INT( ctx->ptr[1] ) : -1 );
		nnc = ( ctx->chars > 2? FB_CHAR_TO_INT( ctx->ptr[2] ) : -1 );

		doexit = FALSE;
		switch( c )
		{
		case '*':
			/* "**..." number format (includes "**$...") */
			if( nc == '*' )
				doexit = TRUE;

			break;

		case '$':
			/* "$$..." number format */
			if( nc == '$' )
				doexit = TRUE;

			break;

		case '+':
			/* "+#...", "+$$...", "+**...", "+.#..." */
			if( (nc == '#') ||
			    ((nc == '$') && (nnc == '$')) ||
			    ((nc == '*') && (nnc == '*')) ||
			    ((nc == '.') && (nnc == '#')) )

				doexit = TRUE;
			break;

		case '!':
		case '\\':
		case '&':
		case '#':
			/* "!", "\ ... \", "&" string formats, "#..." number format */
			doexit = TRUE;
			break;

		case '.':
			/* ".#[...]" number format */
			if( nc == '#' )
				doexit = TRUE;

			break;

		case '_':
			/* escape next char if there is one, otherwise just print '_' */
			if( ctx->chars > 1 )
			{
				c = nc;
				++ctx->ptr;
				--ctx->chars;
			}
			break;

		default:
			break;
		}

		if( doexit )
			break;

		if( ctx->utf8 && c >= 0x80 ) {
			ssize_t consumed = 0;
			ssize_t encoded;
			unsigned int scalar = fb_hUtf8Decode(ctx->ptr, ctx->chars, &consumed);
			encoded = fb_hUtf8Encode(buffer + len, scalar);
			DBG_ASSERT( (encoded >= 1) && (encoded <= 4) );
			len += (int)encoded;
			ctx->ptr += consumed;
			ctx->chars -= consumed;
		} else {
			buffer[len++] = (char)c;
			++ctx->ptr;
			--ctx->chars;
		}
	}

	/* flush */
	if( len > 0 )
	{
		buffer[len] = '\0';
		hPrintText( fnum, buffer, len );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

static int hPrintUsingString( int fnum, FBSTRING_ACCESS *s, int mask, int unicode )
{
	FB_PRINTUSGCTX *ctx;
	char buffer[BUFFERLEN+1];
	int c, nc, doexit;
	ssize_t strchars;

	ctx = FB_TLSGETCTX( PRINTUSG );
	if( unicode ) ctx->utf8 = TRUE;

    /* restart if needed */
	if( ctx->chars == 0 )
	{
		ctx->ptr = ctx->fmtstr.data;
		ctx->chars = FB_STRSIZE( &ctx->fmtstr );
	}

	/* any text first */
	fb_PrintUsingFmtStr( fnum );

	strchars = -1;

	if( ctx->ptr == NULL )
		ctx->chars = 0;

	while( ctx->chars > 0 )
    {
		c = FB_CHAR_TO_INT( *ctx->ptr );
        nc = ( ctx->chars > 1? FB_CHAR_TO_INT( ctx->ptr[1] ) : -1 );

		doexit = TRUE;
		switch( c )
		{
		case '!':
			if( unicode ) {
				hPrintField(fnum, s, 1, TRUE);
				++ctx->ptr;
				--ctx->chars;
				break;
			}
			if( FB_STRSIZE( s ) >= 1 )
				buffer[0] = s->data[0];
			else
				buffer[0] = ' ';

			buffer[1] = '\0';
			hPrintText( fnum, buffer, (ssize_t)strlen(buffer) );

			++ctx->ptr;
			--ctx->chars;
			break;

		case '&':
			hPrintText( fnum, s->data ? s->data : "", FB_STRSIZE(s) );

			++ctx->ptr;
			--ctx->chars;
			break;

		case '\\':
			if( (strchars != -1) || (nc == ' ') || (nc == '\\') )
			{
				if( strchars > 0 )
				{
					++strchars;
					hPrintField(fnum, s, strchars, unicode);

					++ctx->ptr;
					--ctx->chars;
				}
				else
				{
					strchars = 1;
					doexit = FALSE;
				}
			}
			break;

		case ' ':
			if( strchars > -1 )
			{
				++strchars;
				doexit = FALSE;
			}
			break;

		default:
			break;
		}

		if( doexit )
			break;

		++ctx->ptr;
		--ctx->chars;
	}

	/* any text */
	fb_PrintUsingFmtStr( fnum );

	/**/
	if( (unsigned int)mask & FB_PRINT_ISLAST )
	{
		if( mask & FB_PRINT_NEWLINE )
			fb_PrintVoid( fnum, FB_PRINT_NEWLINE );

		fb_StrDelete( &ctx->fmtstr );
		/* The final value can leave an unmatched field. The following
		   USING end call must not revisit the released mask storage. */
		ctx->ptr = NULL;
		ctx->chars = 0;

	}

	/* del if temp */
	fb_hStrDelTemp( s );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_PrintUsingStr( int fnum, FBSTRING_ACCESS *source, int mask )
{
	return hPrintUsingString(fnum, source, mask, FALSE);
}

FBCALL int fb_PrintUsingUstr( int fnum, FBSTRING_ACCESS *source, int mask )
{
	return hPrintUsingString(fnum, source, mask, TRUE);
}

FBCALL int fb_PrintUsingWstr( int fnum, FB_WCHAR *source, int mask )
{
	return fb_PrintUsingUstr(fnum, fb_UStrFromWstr(source), mask);
}

static int hPrintNumber
	(
		int fnum,
		unsigned long long val, int64_t val_exp, int flags,
		int mask
	)
{
	FB_PRINTUSGCTX *ctx;
	char buffer[BUFFERLEN+1], *p;
	int val_digs;
	int64_t val_zdigs;
	uint64_t scientific_exp_magnitude;
	unsigned long long val0;
	int val_digs0;
	int64_t val_exp0;
	int val_isneg, val_isfloat, val_issng;
	int c, lc;
#ifdef DEBUG
	int nc; /* used for sanity checks */
#endif
	int doexit, padchar, expdigs;
	int64_t intdigs, decdigs;
	int adddollar, addcommas, signatend, signatstart, plussign, toobig;
	int expsignchar, decpoint;
	int64_t intdigs2, totdigs;
	int isamp;
	int i;
	uint64_t total_digit_slots;
	uint64_t chars = 0;

	ctx = FB_TLSGETCTX( PRINTUSG );

	/* restart if needed */
	if( ctx->chars == 0 )
	{
		ctx->ptr = ctx->fmtstr.data;
		ctx->chars = FB_STRSIZE( &ctx->fmtstr );
	}

	/* any text first */
	fb_PrintUsingFmtStr( fnum );

	/**/
	padchar     = CHAR_SPACE;
	intdigs     = 0;
	decdigs     = -1;
	expdigs     = 0;
	adddollar   = FALSE;
	addcommas   = FALSE;
	signatend   = FALSE;
	signatstart = FALSE;
	plussign    = FALSE;
	toobig      = 0;
	isamp       = FALSE;

	lc = -1;

	if( ctx->ptr == NULL )
	{
		ctx->chars = 0;
	}

	while( ctx->chars > 0 )
	{
		/* exit if just parsed end '+'/'-' sign, or '&' sign */
		if( signatend || isamp )
		{
			break;
		}

		c = FB_CHAR_TO_INT( *ctx->ptr );
#ifdef DEBUG
		nc = ( ctx->chars > 1? FB_CHAR_TO_INT( *(ctx->ptr+1) ): -1 );
#endif
		doexit = FALSE;
		switch( c )
		{
		case '#':
			/* increment intdigs or decdigs if in int/dec part, else exit */
			if( expdigs != 0 )
			{
				doexit = TRUE;
			}
			else if( decdigs != -1 )
			{
				++decdigs;
			}
			else
			{
				intdigs = hIncrementDigitCount(intdigs);
			}
			break;

		case '.':
			/* add decimal point if still in integer part, else exit */
			if( decdigs != -1 || expdigs != 0 )
			{
				doexit = TRUE;
			}
			else
			{
				decdigs = 0;
			}
			break;

		case '*':
			/* if first two characters, change padding to asterisks, else exit */
			if( (intdigs == 0 && decdigs == -1) )
			{	/* first asterisk */
				DBG_ASSERT( nc == '*' ); /* must be two at start, otherwise we're not parsing a format string and shouldn't have been brought here! */
				padchar = CHAR_STAR;
				intdigs = hIncrementDigitCount(intdigs);
			}
			else if( intdigs == 1 && lc == '*' )
			{	/* second asterisk */
				intdigs = hIncrementDigitCount(intdigs);
			}
			else
				doexit = TRUE;
			break;

		case '$':
			/* at beginning ("$..."), or after two '*'s ("**$..."): prepend a dollar sign to number */

			/* did it follow a '*'? (Will have been the two at the start, else would have exited by now */
			if( lc == '*' )
			{
				adddollar = TRUE;
			}
			/* two at start of number, before integer part? */
			else if( intdigs == 0 && decdigs == -1 )
			{
				if( !adddollar )
				{	/* first dollar */
					DBG_ASSERT( nc == '$' ); /* otherwise we're not parsing a format string and shouldn't have been brought here! */
					adddollar = TRUE;
				}
				else
				{	/* second dollar */
					DBG_ASSERT( lc == '$' );
					intdigs = hIncrementDigitCount(intdigs);
				}
			}
			else
			{
				doexit = TRUE;
			}
			break;

		case ',':
			/* if parsing integer part, enable commas and increment intdigs */
			if( decdigs != -1 || expdigs != 0 )
			{
				doexit = TRUE;
			}
			else
			{
				addcommas = TRUE;
				intdigs = hIncrementDigitCount(intdigs);
			}
			break;

		case '+':
		case '-':
			/* '+' at start/end:  explicit '+'/'-' sign
			   '-' at end:  explicit '-' sign, if negative */

			/* one already at start? */
			if( signatstart )
			{
				doexit = TRUE;
			}
			/* found one before integer part? */
			else if( intdigs == 0 && decdigs == -1 )
			{
				DBG_ASSERT( c != '-' ); /* explicit '-' sign isn't checked for at start */
				if( c == '+' )
				{
					plussign = TRUE;
				}
				signatstart = TRUE;
			}
			/* otherwise it's at the end, as long as there are enough expdigs for an
			   exponent (or none at all), otherwise they are all normal printable characters */
			else if( expdigs == 0 || expdigs >= MIN_EXPDIGS )
			{
				if( c == '+' )
				{
					plussign = TRUE;
				}
				signatend = TRUE;
			}
			else
			{
				doexit = TRUE;
			}
			break;

		case '^':
			/* exponent digits (there must be at least MIN_EXPDIGS of them,
			   otherwise they will just be appended as printable chars      */

			/* Too many? Leave the rest as printable chars */
			if( expdigs < MAX_EXPDIGS )
			{
				++expdigs;
			}
			else
			{
				doexit = TRUE;
			}
			break;

		case '&':
			/* string format '&'
			   print number in most natural form - similar to STR */
			if( intdigs == 0 && decdigs == -1 && !signatstart )
			{
				DBG_ASSERT( expdigs == 0 );
				isamp = TRUE;
			}
			else
			{
				doexit = TRUE;
			}
			break;

		default:
			doexit = TRUE;
		}

		if( doexit )
		{
			break;
		}

		++ctx->ptr;
		--ctx->chars;

		lc = c;
	}

	/* ------------------------------------------------------ */

	/* check flags */
	val_isneg = ( (flags & VAL_ISNEG) != 0 );
	val_isfloat = ( (flags & VAL_ISFLOAT) != 0 );
	val_issng = ( (flags & VAL_ISSNG) != 0 );

	if( (flags & (VAL_ISINF | VAL_ISIND | VAL_ISNAN)) != 0)
	{
		if( (flags & VAL_ISINF) != 0 )
		{
			chars = CHARS_INF;
		}
		else if( (flags & VAL_ISIND) != 0 )
		{
			chars = CHARS_IND;
		}
		else if( (flags & VAL_ISNAN) != 0 )
		{
			chars = CHARS_NAN;
		}
		else
		{
			DBG_ASSERT( 0 );
		}

		/* Set value to 1.1234 (placeholder for "1.#XYZ") */
		val = 11234;
		val_exp = -4;
	}

	if( isamp && (flags & VAL_ISBOOL) != 0 )
	{
		/* String value for '&': return 'true'/'false'
		   (use val to placehold digits) */
		if( val != 0 )
		{
			chars = CHARS_TRUE;
			val = 1234;
			val_isneg = FALSE;
		}
		else
		{
			chars = CHARS_FALSE;
			val = 12345;
		}
		val_exp = 0;
	}

	if( val != 0 )
	{
		val_digs = hNumDigits( val );
	}
	else
	{
		val_digs = 0;
	}
	val_zdigs = 0;

	/* Special '&' format? */
	if( isamp )
	{
		if( val_issng )
		{	/* crop to 7-digit precision */
			if( val_digs > SNG_AUTODIGS )
			{
				val = hDivPow10_ULL( val, val_digs - SNG_AUTODIGS );
				val_exp += val_digs - SNG_AUTODIGS;
				val_digs = SNG_AUTODIGS;
			}

			if( val == 0 )
			{	/* val has been scaled down to zero */
				val_digs = 0;
				val_exp = -decdigs;
			}
			else if( val == hPow10_ULL( val_digs ) )
			{	/* rounding up took val to next power of 10:
				   set value to 1, put val_digs zeroes onto val_exp */
				val = 1;
				val_exp += val_digs;
				val_digs = 1;
			}
		}

		if( val_isfloat )
		{	/* remove trailing zeroes in float digits */
			while( val_digs > 1 && (val % 10) == 0 )
			{
				val /= 10;
				--val_digs;
				++val_exp;
			}
		}

		/* set digits for fixed-point */
		if( hInt64GreaterThanNegativeMagnitude(val_exp,
			(uint64_t)val_digs) )
		{
			intdigs = val_digs + val_exp;
		}
		else
		{
			intdigs = 1;
		}

		if( val_exp < 0 )
		{
			decdigs = -val_exp;
		}

		if( val_isfloat )
		{	/* scientific notation? e.g. 3.1E+42 */
			if( intdigs > 16 || (val_issng && intdigs > 7) ||
			    hInt64LessThanNegativeMagnitude(val_exp,
				(uint64_t)(MIN_EXPDIGS - 1) + (uint64_t)val_digs) )
			{
				intdigs = 1;
				decdigs = val_digs - 1;

				{
					uint64_t exponent_magnitude = hInt64Magnitude(val_exp);

					if (val_digs == 0) {
						if (val_exp > 0)
							scientific_exp_magnitude =
								(uint64_t)val_exp - 1u;
						else
							scientific_exp_magnitude =
								exponent_magnitude + 1u;
					} else {
						uint64_t exponent_offset =
							(uint64_t)(val_digs - 1);

						if (val_exp < 0) {
							if (exponent_magnitude > exponent_offset)
								scientific_exp_magnitude =
									exponent_magnitude - exponent_offset;
							else
								scientific_exp_magnitude =
									exponent_offset - exponent_magnitude;
						} else {
							scientific_exp_magnitude =
								(uint64_t)val_exp + exponent_offset;
						}
					}
				}
				expdigs = 2 + hNumDigits(
					(unsigned long long)scientific_exp_magnitude);
				if( expdigs < MIN_EXPDIGS + 1 )
					expdigs = MIN_EXPDIGS;
			}
		}

		if( val_isneg )
		{
			signatstart = TRUE;
		}
	}

	/* crop number of digits */
	total_digit_slots = (uint64_t)intdigs;
	if( decdigs >= 0 )
		total_digit_slots += (uint64_t)decdigs + 1u;
	if( total_digit_slots > (uint64_t)MAX_DIGS )
	{
		decdigs -= (int64_t)(total_digit_slots - (uint64_t)MAX_DIGS);
		if( (decdigs < 0) && (decdigs != -1) )
		{
			intdigs -= (-1 - decdigs);
			decdigs = -1;
		}
	}

	/* decimal point if decdigs >= 0 */
	if( decdigs < 0 )
	{
		decpoint = FALSE;
		decdigs = 0;
	}
	else
	{
		decpoint = TRUE;
	}

	/* ------------------------------------------------------ */

	p = &buffer[BUFFERLEN];
	ADD_CHAR( '\0' );

	if( signatend )
	{	/* put sign at end */
		if( val_isneg )
		{
			ADD_CHAR( CHAR_MINUS );
		}
		else
		{
			ADD_CHAR( plussign? CHAR_PLUS : CHAR_SPACE );
		}
	}
	else if( val_isneg && !signatstart )
	{	/* implicit negative sign at start */
		signatstart = TRUE;
		--intdigs;
	}

	/* fixed-point format? */
	if( expdigs < MIN_EXPDIGS )
	{
		/* append any trailing carets */
		for( ; expdigs > 0; --expdigs )
		{
			ADD_CHAR( '^' );
		}

		/* backup unscaled value */
		val0 = val;
		val_digs0 = val_digs;
		val_exp0 = val_exp;

		/* check range */
		if( hInt64LessThanNegativeMagnitude(val_exp,
			(uint64_t)decdigs) )
		{	/* scale and round integer value to get val_exp equal to -decdigs */
			int scale = (int)(-decdigs - val_exp0);
			val_exp += scale;
			val_digs -= scale;
			val = hDivPow10_ULL( val, scale );

			if( val == 0 )
			{	/* val is/has been scaled down to zero */
				val_digs = 0;
				val_exp = -decdigs;
			}
			else if( val == hPow10_ULL( val_digs ) )
			{	/* rounding up took val to next power of 10:
				   set value to 1, put val_digs zeroes onto val_exp */
				val = 1;
				val_exp += val_digs;
				val_digs = 1;
			}
		}

		if (hInt64LessThanNegativeMagnitude(val_exp, (uint64_t)val_digs))
			intdigs2 = 0;
		else if ((val_exp >= 0) &&
			((uint64_t)val_exp >
			 (uint64_t)INT64_MAX - (uint64_t)val_digs))
			intdigs2 = INT64_MAX;
		else
			intdigs2 = (int64_t)val_digs + val_exp;

		if (addcommas && (intdigs2 > 0)) {
			uint64_t digit_slots = (uint64_t)intdigs2;
			uint64_t comma_slots = (digit_slots - 1u) / 3u;

			if (comma_slots > (uint64_t)INT64_MAX - digit_slots)
				intdigs2 = INT64_MAX;
			else
				intdigs2 = (int64_t)(digit_slots + comma_slots);
		}

		/* compare fixed/floating point representations,
		   and use the one that needs fewest digits */
		if( (intdigs2 > intdigs) &&
		    /* The order check keeps this mathematical difference nonnegative;
	       unsigned arithmetic avoids signed overflow for large digit counts. */
		    ((uint64_t)intdigs2 - (uint64_t)intdigs >
		     (uint64_t)MIN_EXPDIGS) )
		{	/* too many digits in number for fixed point:
			   switch to floating-point */

			expdigs = MIN_EXPDIGS; /* add three digits for exp notation (was four in QB) */
			toobig = 1;  /* add '%' sign */

			/* restore unscaled value */
			val = val0;
			val_digs = val_digs0;
			val_exp = val_exp0;

			val_zdigs = 0;
		}
		else
		{	/* keep fixed point */

			if( intdigs2 > intdigs )
			{	/* slightly too many digits in number */
				intdigs = intdigs2; /* extend intdigs */
				toobig = 1;         /* add '%' sign */
			}

			if( hInt64GreaterThanNegativeMagnitude(val_exp,
				(uint64_t)decdigs) )
			{	/* put excess trailing zeroes from val_exp into val_zdigs */
				val_zdigs = val_exp - -decdigs;
				val_exp = -decdigs;
			}
		}
	}


	/* floating-point format */
	if( expdigs > 0 )
	{
		addcommas = FALSE; /* commas unused in f-p format */

		if( intdigs == -1 || (intdigs == 0 && decdigs == 0) )
		{	/* add [another] '%' sign */
			intdigs = hIncrementDigitCount(intdigs);
#if 0
			++toobig;   /* QB could prepend two independent '%'s */
#else
			toobig = 1; /* We'll just stick with one */
#endif
		}

		totdigs = intdigs + decdigs; /* treat intdigs and decdigs the same */
		val_exp += decdigs; /* move decimal position to end */

		/* blank first digit if positive and no explicit sign
		   (pos/neg numbers should be formatted the same where
		   possible, as in QB) */
		if( !isamp && !val_isneg && !(signatstart || signatend) )
		{
			if( intdigs >= 1 && totdigs > 1 )
			{
				--totdigs;
			}
		}

		if( val == 0 )
		{
			val_exp = 0;         /* ensure exponent is printed as 0 */
			val_zdigs = decdigs; /* enough trailing zeroes to fill dec part */
		}
		else if( val_digs < totdigs )
		{	/* add "zeroes" to the end of val:
			   subtract from val_exp and put into val_zdigs */
			val_zdigs = totdigs - val_digs ;
			val_exp -= val_zdigs;
		}
		else if( val_digs > totdigs )
		{	/* scale down value */
			int scale = (int)((int64_t)val_digs - totdigs);
			val = hDivPow10_ULL( val, scale );
			val_exp += scale;
			val_digs = (int)totdigs;
			val_zdigs = 0;

			if( val >= hPow10_ULL( val_digs ) )
			{	/* rounding up brought val to the next power of 10:
				   add the extra digit onto val_exp */
				val /= 10;
				++val_exp;
			}
		}
		else
		{
			val_zdigs = 0;
		}


		/* output exp part */

		if( val_exp < 0 )
		{
			expsignchar = CHAR_MINUS;
			val_exp = -val_exp;
		}
		else
		{
			expsignchar = CHAR_PLUS;
		}

		/* expdigs > 3 */
		for( ; expdigs > 3; --expdigs )
		{
			ADD_CHAR( CHAR_ZERO + (val_exp % 10) );
			val_exp /= 10;
		}

		/* expdigs == 3 */
		if( val_exp > 9 ) /* too many exp digits? */
		{
#if 1		/* Add remaining digits (QB would just crop these) */
			do {
				ADD_CHAR( CHAR_ZERO + (val_exp % 10) );
				val_exp /= 10;
			} while( val_exp > 9 );
			ADD_CHAR( CHAR_ZERO + val_exp );
#endif
			ADD_CHAR( CHAR_TOOBIG ); /* add a '%' sign */
		}
		else
		{
			ADD_CHAR( CHAR_ZERO + val_exp );
		}

#ifdef DEBUG
		expdigs -= 1;
#endif

		/* expdigs == 2 */
		ADD_CHAR( expsignchar );
		ADD_CHAR( CHAR_EXP_SINGLE ); /* QB would use 'D' for doubles */

#ifdef DEBUG
		expdigs -= 2;
#endif
	}


	/* INF/IND/NAN: characters truncated? */
	if( chars != 0 && val_digs < 5 && (flags & VAL_ISBOOL) == 0)
	{
		/* QB wouldn't add the '%'.  But otherwise "#" will result in
		   an innocent-looking "1".  Also, QB corrupts the string data
		   when truncated, so some deviation is desirable anyway) */
		toobig = 1;

		if ( val_digs > 1 )
		{
			chars = CHARS_TRUNC >> (8 * (5 - val_digs));
		}
		else
		{
			chars = 0;
		}
	}


	/* output dec part */
	if( decpoint )
	{
		for( ; decdigs > 0; --decdigs )
		{
			if( val_zdigs > 0 )
			{
				ADD_CHAR( CHAR_ZERO );
				--val_zdigs;
			}
			else if( val_digs > 0 )
			{
				DBG_ASSERT( val > 0 );
				if( chars != 0 )
				{
					ADD_CHAR( chars & 0xff );
					chars >>= 8;
				}
				else
				{
					ADD_CHAR( CHAR_ZERO + (val % 10) );
				}
				val /= 10;
				--val_digs;
			}
			else
			{
				ADD_CHAR( CHAR_ZERO );
			}
		}
		ADD_CHAR( CHAR_DOT );
	}


	/* output int part */
	i = 0;
	for( ;; )
	{
		if( addcommas && (i & 3) == 3 && val_digs > 0 )
		{	/* insert comma */
			ADD_CHAR( CHAR_COMMA );
		}
		else if( val_zdigs > 0 )
		{
			ADD_CHAR( CHAR_ZERO );
			--val_zdigs;
		}
		else if( val_digs > 0 )
		{
			DBG_ASSERT( val > 0 );
			if( chars != 0 )
			{
				ADD_CHAR( chars & 0xff );
				chars >>= 8;
			}
			else
			{
				ADD_CHAR( CHAR_ZERO + (val % 10) );
			}
			val /= 10;
			--val_digs;
		}
		else
		{
			if( i == 0 && intdigs > 0 )
			{
				ADD_CHAR( CHAR_ZERO );
			}
			else
			{
				break;
			}
		}
		DBG_ASSERT( intdigs > 0 );
		++i;
		intdigs = (int64_t)((uint64_t)intdigs - UINT64_C(1));
	}

	DBG_ASSERT( val == 0 );
	DBG_ASSERT( val_digs == 0 );
	DBG_ASSERT( val_zdigs == 0 );

	DBG_ASSERT( decdigs == 0 );
	DBG_ASSERT( expdigs == 0 );
	DBG_ASSERT( intdigs >= 0 );

	/* output dollar sign? */
	if( adddollar )
	{
		ADD_CHAR( CHAR_DOLLAR );
	}

	/* output sign? */
	if( signatstart )
	{
		if( val_isneg )
		{
			ADD_CHAR( CHAR_MINUS );
		}
		else
		{
			ADD_CHAR( plussign? CHAR_PLUS : padchar );
		}
	}

	/* output padding for any remaining intdigs */
	for( ; intdigs > 0; --intdigs )
	{
		ADD_CHAR( padchar );
	}

	/* output '%' sign(s)? */
	for( ; toobig > 0; --toobig )
	{
		ADD_CHAR( CHAR_TOOBIG );
	}


	/**/
	++p;
	hPrintText( fnum, p, (ssize_t)strlen(p) );

	/* ------------------------------------------------------ */

	/* any text */
	fb_PrintUsingFmtStr( fnum );

	/**/
	if( mask & (FB_PRINT_NEWLINE | FB_PRINT_PAD) )
	{
		fb_PrintVoid( fnum, mask & (FB_PRINT_NEWLINE | FB_PRINT_PAD) );
	}

	if( (unsigned int)mask & FB_PRINT_ISLAST )
	{
		fb_StrDelete( &ctx->fmtstr );
		/* The final value can leave an unmatched field. The following
		   USING end call must not revisit the released mask storage. */
		ctx->ptr = NULL;
		ctx->chars = 0;
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

static unsigned long long hScaleDoubleToULL( double value, int64_t *pval_exp )
{
	DBG_ASSERT( value >= 0.0 );

#if 0
	/* scale down to a 16-digit number, plus base-10 exponent */

	if( value == 0.0 )
	{
		*pval_exp = 0;
		return 0;
	}
	long double val_ld = value;
	unsigned long long val_ull;
	int val_exp;

	/* find number of digits in double (approximation, may be 1 lower) */

	val_exp = 1 + (int)floor( log10( val_ld ) - 0.5 );

	/* scale down to 16..17 digits (use long doubles to prevent inaccuracy/overflow in pow) */
	val_exp -= 16;
	val_ld /= pow( (long double)10.0, val_exp );
	if( val_ld >= (long double)1.E+16 )
	{
		val_ld /= (long double)10.0;
		++val_exp;
	}

	/* convert to ULL */
	val_ull = (unsigned long long)(val_ld + 0.5);
	DBG_ASSERT( val_ull >= (unsigned long long)1.E+15 || val_ull == 0 );
	DBG_ASSERT( val_ull <= (unsigned long long)1.E+16 );

	*pval_exp = val_exp;
	return val_ull;
#else

	/*(assumes x86 endian, IEEE-754 floating-point format) */

	unsigned long long val_ull;
	unsigned int exponent;
	int64_t pow2, pow10;
	unsigned int steps;

	val_ull = hDoubleToLongBits( value );
	/* The exponent occupies bits 52 through 62; bit 63 is the sign. */
	exponent = (unsigned int)((val_ull >> 52) & 0x7ffull);
	pow2 = (int64_t)exponent - 1023;
	val_ull &= (1ull << 52)-1;

	if( exponent != 0u )
	{	/* normalized */
		val_ull |= (1ull << 52);
	}
	else
	{	/* denormed */
		pow2 += 1;
	}
	pow2 -= 52; /* 52 (+1?) integer bits in val_ull */

	pow10 = 0;

	if( pow2 > 0 )
	{
		steps = (unsigned int)pow2;
		while( steps > 0u )
		{
			/* essentially, val_ull*=2, --pow2,
			 * dividing by 5 when necessary to keep within 64 bits) */
			if( val_ull < (1ull << 63) )
			{
				val_ull *= 2;
				steps--;
			}
			else
			{
				/* divide by 5, rounding to nearest
				 * (val_ull will be much bigger than 3 so no underflow) */
				val_ull = (val_ull - 3) / 5 + 1;
				++pow10;
				steps--;
			}
		}
	}

	else if( pow2 < 0 )
	{
		steps = 0u - (unsigned int)pow2;
		while( steps > 0u )
		{
			/* essentially, val_ull/=2, ++pow2,
			 * multiplying by 5 when possible to keep precision high */
			if( val_ull <= 0x3333333333333333ull )
			{	/* multiply by 5 (max 0xffffffffffffffff) */
				val_ull *= 5;
				--pow10;
				steps--;
			}
			else
			{	/* divide by 2, rounding to even */
				val_ull = val_ull / 2 + (val_ull & (val_ull / 2) & 1);
				steps--;
			}
		}
	}

	if( val_ull >= hPow10_ULL(DBL_MAXDIGS) )
	{	/* scale to 16 digits */

		int scale = hNumDigits(val_ull) - DBL_MAXDIGS;
		val_ull = hDivPow10_ULL( val_ull, scale );
		pow10 += scale;

		DBG_ASSERT( val_ull <= hPow10_ULL( DBL_MAXDIGS ) );
	}

	*pval_exp = pow10;
	return val_ull;

#endif
}

FBCALL int fb_PrintUsingDouble( int fnum, double value, int mask )
{
	int64_t val_exp = 0;
	int flags;
	unsigned long long val_ull = 1;

	flags = VAL_ISFLOAT;

	if( hIsNeg( value ) )
		flags |= VAL_ISNEG;

	if( hIsZero( value ) )
	{
		val_ull = 0;
		val_exp = 0;
	}
	else if( hIsFinite( value ) )
	{
		value = fabs( value );
		val_ull = hScaleDoubleToULL( value, &val_exp );
	}
	else
	{
		if( hIsInf( value ) )
			flags |= VAL_ISINF;
		else if( hIsInd( value ) )
			flags |= VAL_ISIND;
		else if( hIsNan( value ) )
			flags |= VAL_ISNAN;
		else
			DBG_ASSERT( 0 );
	}

	return hPrintNumber( fnum, val_ull, val_exp, flags, mask );
}

FBCALL int fb_PrintUsingSingle( int fnum, float value_f, int mask )
{
	int64_t val_exp = 0;
	int flags;
	unsigned long long val_ull = 1;

	flags = VAL_ISFLOAT | VAL_ISSNG;

	if( hIsNeg( value_f ) )
		flags |= VAL_ISNEG;

	if( hIsZero( value_f ) )
	{
		val_ull = 0;
		val_exp = 0;
	}
	else if( hIsFinite( value_f ) )
	{
		value_f = fabsf( value_f );
		val_ull = hScaleDoubleToULL( value_f, &val_exp );
	}
	else
	{
		if( hIsInf( value_f ) )
			flags |= VAL_ISINF;
		else if( hIsInd( value_f ) )
			flags |= VAL_ISIND;
		else if( hIsNan( value_f ) )
			flags |= VAL_ISNAN;
		else
			DBG_ASSERT( 0 );
	}

	return hPrintNumber( fnum, val_ull, val_exp, flags, mask );
}

FBCALL int fb_PrintUsingULongint( int fnum, unsigned long long value_ull, int mask )
{
	return hPrintNumber( fnum, value_ull, 0, 0, mask );
}

FBCALL int fb_PrintUsingLongint( int fnum, long long val_ll, int mask )
{
	int flags;
	unsigned long long val_ull;

	if( val_ll < 0 )
	{
		flags = VAL_ISNEG;
		/* Unsigned subtraction gives LLONG_MIN its representable magnitude. */
		val_ull = 0ull - (unsigned long long)val_ll;
	}
	else
	{
		flags = 0;
		val_ull = (unsigned long long)val_ll;
	}

	return hPrintNumber( fnum, val_ull, 0, flags, mask );
}

FBCALL int fb_PrintUsingBoolean( int fnum, char val, int mask )
{
	int flags = VAL_ISBOOL;
	unsigned long long val_ull;

	if( val != 0 )
	{
		flags |= VAL_ISNEG;
		val_ull = 1ull;
	}
	else
	{
		val_ull = 0ull;
	}

	return hPrintNumber( fnum, val_ull, 0, flags, mask );
}

/* end of console/io_printusg.c */
