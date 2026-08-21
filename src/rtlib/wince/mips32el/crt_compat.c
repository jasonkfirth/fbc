/*
 * FreeBASIC Windows CE MIPS runtime
 * ---------------------------------
 *
 * File: wince/mips32el/crt_compat.c
 *
 * Purpose:
 *
 *     Supply the small ISO C compatibility surface omitted by MIPS COREDLL.
 *
 * Responsibilities:
 *
 *     - adapt missing single-precision functions to COREDLL's double API
 *     - provide the omitted time and local-time entry points
 *     - provide the omitted C99 parsing and rounding entry points
 *     - bridge MinGW's strtod helper to COREDLL's exported spelling
 *     - expose the legacy hypot spelling expected by MinGW headers
 *     - classify IEEE-754 float and double values without an FPU
 *     - route C assertion failures through the FreeBASIC runtime
 *
 * This file intentionally does NOT contain:
 *
 *     - a general-purpose mathematical library
 *     - compiler software-floating-point helpers
 *     - compatibility code for ARM or desktop Windows
 *     - emulator or device-specific behavior
 */

#include "../../fb.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <windows.h>


/* ------------------------------------------------------------------------- */
/* COREDLL C runtime adapters                                                */
/* ------------------------------------------------------------------------- */

#define FB_FILETIME_TICKS_PER_SECOND UINT64_C( 10000000 )
#define FB_FILETIME_UNIX_EPOCH UINT64_C( 116444736000000000 )

extern char *__cdecl fb_hCoredllStrdup( const char *text )
	__asm__( "_strdup" );
extern wchar_t *__cdecl fb_hCoredllWcsdup( const wchar_t *text )
	__asm__( "_wcsdup" );

double __cdecl __strtod( const char *text, char **endptr )
{
	size_t length;
	size_t index;
	wchar_t *wide_end = NULL;
	wchar_t *wide_text;
	double result;

	if( text == NULL ) {
		if( endptr != NULL )
			*endptr = NULL;
		return 0.0;
	}

	length = strlen( text );
	if( length > (SIZE_MAX / sizeof( wchar_t )) - 1 ) {
		if( endptr != NULL )
			*endptr = (char *)text;
		return 0.0;
	}

	wide_text = malloc( (length + 1) * sizeof( wchar_t ) );
	if( wide_text == NULL ) {
		if( endptr != NULL )
			*endptr = (char *)text;
		return 0.0;
	}

	/*
	 * MIPS COREDLL's narrow strtod export does not return under the CE6
	 * Rockhopper image, while wcstod is functional.  Numeric syntax consumed
	 * by the C runtime is ASCII, so widening each byte preserves both parsing
	 * behavior and the end-pointer offset without depending on a locale.
	 */
	for( index = 0; index <= length; index++ )
		wide_text[index] = (unsigned char)text[index];

	result = wcstod( wide_text, endptr != NULL ? &wide_end : NULL );
	if( endptr != NULL )
		*endptr = (char *)text + (wide_end - wide_text);

	free( wide_text );
	return result;
}

float __cdecl strtof( const char *text, char **endptr )
{
	return (float)__strtod( text, endptr );
}

long double __cdecl strtold( const char *text, char **endptr )
{
	return (long double)__strtod( text, endptr );
}

char *__cdecl strdup( const char *text )
{
	return fb_hCoredllStrdup( text );
}

wchar_t *__cdecl wcsdup( const wchar_t *text )
{
	return fb_hCoredllWcsdup( text );
}

static int fb_hAsciiDigitValue( unsigned char character )
{
	if( character >= '0' && character <= '9' )
		return character - '0';
	if( character >= 'A' && character <= 'Z' )
		return character - 'A' + 10;
	if( character >= 'a' && character <= 'z' )
		return character - 'a' + 10;
	return -1;
}

unsigned long long __cdecl strtoull( const char *text, char **endptr,
	int base )
{
	const char *cursor = text;
	const char *digits_begin;
	uint64_t value = 0;
	int negative = FALSE;
	int overflow = FALSE;

	if( text == NULL || (base != 0 && (base < 2 || base > 36)) ) {
		if( endptr != NULL )
			*endptr = (char *)text;
		return 0;
	}

	while( *cursor == ' ' || (*cursor >= '\t' && *cursor <= '\r') )
		cursor++;
	if( *cursor == '+' || *cursor == '-' ) {
		negative = (*cursor == '-');
		cursor++;
	}

	if( (base == 0 || base == 16) && cursor[0] == '0' &&
		(cursor[1] == 'x' || cursor[1] == 'X') ) {
		base = 16;
		cursor += 2;
	} else if( base == 0 ) {
		base = (cursor[0] == '0') ? 8 : 10;
	}
	digits_begin = cursor;

	for( ; ; cursor++ ) {
		int digit = fb_hAsciiDigitValue( (unsigned char)*cursor );

		if( digit < 0 || digit >= base )
			break;
		if( value > (UINT64_MAX - (unsigned)digit) / (unsigned)base )
			overflow = TRUE;
		else if( overflow == FALSE )
			value = value * (unsigned)base + (unsigned)digit;
	}

	if( cursor == digits_begin ) {
		if( endptr != NULL )
			*endptr = (char *)text;
		return 0;
	}
	if( endptr != NULL )
		*endptr = (char *)cursor;
	if( overflow )
		return UINT64_MAX;
	if( negative )
		value = UINT64_C( 0 ) - value;
	return value;
}

unsigned long long __cdecl wcstoull( const wchar_t *text, wchar_t **endptr,
	int base )
{
	const wchar_t *cursor = text;
	const wchar_t *digits_begin;
	uint64_t value = 0;
	int negative = FALSE;
	int overflow = FALSE;

	if( text == NULL || (base != 0 && (base < 2 || base > 36)) ) {
		if( endptr != NULL )
			*endptr = (wchar_t *)text;
		return 0;
	}

	while( *cursor == L' ' || (*cursor >= L'\t' && *cursor <= L'\r') )
		cursor++;
	if( *cursor == L'+' || *cursor == L'-' ) {
		negative = (*cursor == L'-');
		cursor++;
	}

	if( (base == 0 || base == 16) && cursor[0] == L'0' &&
		(cursor[1] == L'x' || cursor[1] == L'X') ) {
		base = 16;
		cursor += 2;
	} else if( base == 0 ) {
		base = (cursor[0] == L'0') ? 8 : 10;
	}
	digits_begin = cursor;

	for( ; ; cursor++ ) {
		int digit = fb_hAsciiDigitValue( (unsigned char)*cursor );

		if( *cursor > 0x7f || digit < 0 || digit >= base )
			break;
		if( value > (UINT64_MAX - (unsigned)digit) / (unsigned)base )
			overflow = TRUE;
		else if( overflow == FALSE )
			value = value * (unsigned)base + (unsigned)digit;
	}

	if( cursor == digits_begin ) {
		if( endptr != NULL )
			*endptr = (wchar_t *)text;
		return 0;
	}
	if( endptr != NULL )
		*endptr = (wchar_t *)cursor;
	if( overflow )
		return UINT64_MAX;
	if( negative )
		value = UINT64_C( 0 ) - value;
	return value;
}

static uint64_t fb_hFileTimeTicks( const FILETIME *file_time )
{
	return ((uint64_t)file_time->dwHighDateTime << 32) |
		(uint64_t)file_time->dwLowDateTime;
}

static void fb_hTicksToFileTime( uint64_t ticks, FILETIME *file_time )
{
	file_time->dwLowDateTime = (DWORD)ticks;
	file_time->dwHighDateTime = (DWORD)(ticks >> 32);
}

static int fb_hIsLeapYear( int year )
{
	return ((year % 4) == 0) &&
		(((year % 100) != 0) || ((year % 400) == 0));
}

static int fb_hDayOfYear( const SYSTEMTIME *system_time )
{
	static const unsigned short days_before_month[2][12] = {
		{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
		{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
	};
	int leap = fb_hIsLeapYear( system_time->wYear );

	return days_before_month[leap][system_time->wMonth - 1] +
		system_time->wDay - 1;
}

time_t __cdecl time( time_t *result_pointer )
{
	SYSTEMTIME system_time;
	FILETIME file_time;
	uint64_t ticks;
	time_t result;

	GetSystemTime( &system_time );
	if( SystemTimeToFileTime( &system_time, &file_time ) == FALSE )
		result = (time_t)-1;
	else {
		ticks = fb_hFileTimeTicks( &file_time );
		if( ticks < FB_FILETIME_UNIX_EPOCH )
			result = (time_t)-1;
		else
			result = (time_t)((ticks - FB_FILETIME_UNIX_EPOCH) /
				FB_FILETIME_TICKS_PER_SECOND);
	}

	if( result_pointer != NULL )
		*result_pointer = result;
	return result;
}

struct tm *__cdecl localtime( const time_t *timer )
{
	static struct tm result;
	TIME_ZONE_INFORMATION timezone;
	SYSTEMTIME system_time;
	FILETIME file_time;
	int64_t ticks;
	long bias;
	DWORD timezone_state;

	if( timer == NULL )
		return NULL;

	ticks = (int64_t)FB_FILETIME_UNIX_EPOCH +
		(int64_t)(*timer) * (int64_t)FB_FILETIME_TICKS_PER_SECOND;
	if( ticks < 0 )
		return NULL;

	timezone_state = GetTimeZoneInformation( &timezone );
	bias = timezone.Bias;
	if( timezone_state == TIME_ZONE_ID_STANDARD )
		bias += timezone.StandardBias;
	else if( timezone_state == TIME_ZONE_ID_DAYLIGHT )
		bias += timezone.DaylightBias;

	/* Windows expresses the bias as UTC minus local time. */
	ticks -= (int64_t)bias * 60 * (int64_t)FB_FILETIME_TICKS_PER_SECOND;
	if( ticks < 0 )
		return NULL;

	fb_hTicksToFileTime( (uint64_t)ticks, &file_time );
	if( FileTimeToSystemTime( &file_time, &system_time ) == FALSE )
		return NULL;

	result.tm_sec = system_time.wSecond;
	result.tm_min = system_time.wMinute;
	result.tm_hour = system_time.wHour;
	result.tm_mday = system_time.wDay;
	result.tm_mon = system_time.wMonth - 1;
	result.tm_year = system_time.wYear - 1900;
	result.tm_wday = system_time.wDayOfWeek;
	result.tm_yday = fb_hDayOfYear( &system_time );
	result.tm_isdst = (timezone_state == TIME_ZONE_ID_DAYLIGHT);
	return &result;
}


/* ------------------------------------------------------------------------- */
/* COREDLL mathematical adapters                                             */
/* ------------------------------------------------------------------------- */

extern double __cdecl _hypot( double x, double y );

/*
    Windows CE inverse cosine

    COREDLL's MIPS acos() returns some ordinary finite values one ULP below
    the correctly rounded result.  Besides affecting programs directly, that
    disagrees with fbc's compile-time folding of the same expression.

    This implementation follows fdlibm's piecewise approximation.  The
    original algorithm and constants are Copyright (C) 1993 Sun Microsystems,
    Inc.; permission to use, copy, modify, and distribute them is freely
    granted provided that notice is preserved.

    The positive branch clears the low 32 bits of a temporary double.  This
    file is exclusively for little-endian MIPS, so bytes zero through three
    are the low word.  Byte access also avoids an unaligned or aliasing-unsafe
    integer access.
*/
double __cdecl acos( double x )
{
	static const double one = 1.00000000000000000000e+00;
	static const double pi = 3.14159265358979311600e+00;
	static const double pio2_hi = 1.57079632679489655800e+00;
	static const double pio2_lo = 6.12323399573676603587e-17;
	static const double pS0 = 1.66666666666666657415e-01;
	static const double pS1 = -3.25565818622400915405e-01;
	static const double pS2 = 2.01212532134862925881e-01;
	static const double pS3 = -4.00555345006794114027e-02;
	static const double pS4 = 7.91534994289814532176e-04;
	static const double pS5 = 3.47933107596021167570e-05;
	static const double qS1 = -2.40339491173441421878e+00;
	static const double qS2 = 2.02094576023350569471e+00;
	static const double qS3 = -6.88283971605453293030e-01;
	static const double qS4 = 7.70381505559019352791e-02;
	double z;
	double p;
	double q;
	double r;
	double w;
	double s;
	double c;
	double df;
	unsigned char *df_bytes;
	volatile double invalid;

	if( x != x ) {
		return x;
	}

	if( x >= one ) {
		if( x == one ) {
			return 0.0;
		}

		invalid = x - x;
		return invalid / invalid;
	}

	if( x <= -one ) {
		if( x == -one ) {
			return pi + (2.0 * pio2_lo);
		}

		invalid = x - x;
		return invalid / invalid;
	}

	if( (x > -0.5) && (x < 0.5) ) {
		/* 0x1p-57 is the point below which x cannot affect pi / 2. */
		if( (x >= -0x1.0p-57) && (x <= 0x1.0p-57) ) {
			return pio2_hi + pio2_lo;
		}

		z = x * x;
		p = z * (pS0 + z * (pS1 + z *
		    (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
		q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
		r = p / q;
		return pio2_hi - (x - (pio2_lo - (x * r)));
	}

	if( x < 0.0 ) {
		z = (one + x) * 0.5;
		p = z * (pS0 + z * (pS1 + z *
		    (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
		q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
		s = sqrt( z );
		r = p / q;
		w = (r * s) - pio2_lo;
		return pi - (2.0 * (s + w));
	}

	z = (one - x) * 0.5;
	s = sqrt( z );
	df = s;
	df_bytes = (unsigned char *)&df;
	df_bytes[0] = 0;
	df_bytes[1] = 0;
	df_bytes[2] = 0;
	df_bytes[3] = 0;
	c = (z - (df * df)) / (s + df);
	p = z * (pS0 + z * (pS1 + z *
	    (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
	q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
	r = p / q;
	w = (r * s) + c;
	return 2.0 * (df + w);
}

float __cdecl acosf( float x )
{
	return (float)acos( (double)x );
}

float __cdecl asinf( float x )
{
	return (float)asin( (double)x );
}

float __cdecl atanf( float x )
{
	return (float)atan( (double)x );
}

float __cdecl atan2f( float y, float x )
{
	return (float)atan2( (double)y, (double)x );
}

float __cdecl cosf( float x )
{
	return (float)cos( (double)x );
}

float __cdecl logf( float x )
{
	return (float)log( (double)x );
}

float __cdecl log10f( float x )
{
	return (float)log10( (double)x );
}

float __cdecl sinf( float x )
{
	return (float)sin( (double)x );
}

float __cdecl tanf( float x )
{
	return (float)tan( (double)x );
}

double __cdecl hypot( double x, double y )
{
	return _hypot( x, y );
}

double __cdecl rint( double x )
{
	return nearbyint( x );
}

float __cdecl rintf( float x )
{
	return nearbyintf( x );
}

long double __cdecl rintl( long double x )
{
	return (long double)nearbyint( (double)x );
}

long __cdecl lrint( double x )
{
	return (long)nearbyint( x );
}

long __cdecl lrintf( float x )
{
	return (long)nearbyintf( x );
}

long __cdecl lrintl( long double x )
{
	return (long)nearbyint( (double)x );
}

long long __cdecl llrint( double x )
{
	return (long long)nearbyint( x );
}

long long __cdecl llrintf( float x )
{
	return (long long)nearbyintf( x );
}

long long __cdecl llrintl( long double x )
{
	return (long long)nearbyint( (double)x );
}


/* ------------------------------------------------------------------------- */
/* ISO C bounded formatting                                                  */
/* ------------------------------------------------------------------------- */

static int fb_hVsnprintfAttempt( char *buffer, size_t buffer_size,
	const char *format, va_list arguments )
{
	va_list copy;
	int result;

	va_copy( copy, arguments );
	result = _vsnprintf( buffer, buffer_size, format, copy );
	va_end( copy );

	if( buffer != NULL && buffer_size > 0 )
		buffer[buffer_size - 1] = '\0';

	return result;
}

int __cdecl vsnprintf( char *buffer, size_t buffer_size, const char *format,
	va_list arguments )
{
	char *measurement_buffer;
	size_t measurement_size;
	int result;

	if( buffer == NULL && buffer_size != 0 )
		return -1;

	if( buffer_size != 0 ) {
		result = fb_hVsnprintfAttempt( buffer, buffer_size, format, arguments );
		if( result >= 0 )
			return result;
	}

	/* Old COREDLL _vsnprintf returns -1 after truncation instead of the C99
	   output length.  Retry in a private growing buffer so callers still get
	   the standard result, including size-query calls with a null buffer. */
	measurement_size = (buffer_size < 128) ? 128 : buffer_size;
	for( ; ; ) {
		measurement_buffer = (char *)malloc( measurement_size );
		if( measurement_buffer == NULL )
			return -1;

		result = fb_hVsnprintfAttempt( measurement_buffer, measurement_size,
			format, arguments );
		free( measurement_buffer );
		if( result >= 0 )
			return result;

		if( measurement_size > (size_t)INT_MAX / 2 )
			return -1;
		measurement_size *= 2;
	}
}

int __cdecl snprintf( char *buffer, size_t buffer_size, const char *format,
	... )
{
	va_list arguments;
	int result;

	va_start( arguments, format );
	result = vsnprintf( buffer, buffer_size, format, arguments );
	va_end( arguments );
	return result;
}


/* ------------------------------------------------------------------------- */
/* IEEE-754 classification                                                   */
/* ------------------------------------------------------------------------- */

static int fb_hClassifyIeee754( uint64_t bits, uint64_t fraction_mask,
	uint64_t exponent_mask )
{
	uint64_t magnitude = bits & (fraction_mask | exponent_mask);
	uint64_t exponent = magnitude & exponent_mask;

	if( exponent == exponent_mask )
		return (magnitude & fraction_mask) ? FP_NAN : FP_INFINITE;

	if( exponent == 0 )
		return (magnitude & fraction_mask) ? FP_SUBNORMAL : FP_ZERO;

	return FP_NORMAL;
}

int __cdecl __fpclassifyf( float x )
{
	union {
		float value;
		uint32_t bits;
	} representation;

	representation.value = x;
	return fb_hClassifyIeee754( representation.bits, UINT32_C( 0x007fffff ),
		UINT32_C( 0x7f800000 ) );
}

int __cdecl __fpclassify( double x )
{
	union {
		double value;
		uint64_t bits;
	} representation;

	representation.value = x;
	return fb_hClassifyIeee754( representation.bits,
		UINT64_C( 0x000fffffffffffff ), UINT64_C( 0x7ff0000000000000 ) );
}

int __cdecl __fpclassifyl( long double x )
{
	/* The Windows MIPS ABI represents long double with the IEEE-754 binary64
	   format, matching the double type used by COREDLL. */
	return __fpclassify( (double)x );
}


/* ------------------------------------------------------------------------- */
/* Assertion compatibility                                                   */
/* ------------------------------------------------------------------------- */

void __cdecl _assert( const char *expression, const char *filename,
	int line_number )
{
	fb_Assert( (char *)filename, line_number, "C runtime", (char *)expression );

	/* fb_Assert terminates through fb_End.  Preserve assert.h's noreturn
	   contract even if a future runtime exit hook unexpectedly returns. */
	for( ; ; ) {
	}
}

/* end of wince/mips32el/crt_compat.c */
