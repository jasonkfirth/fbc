/*
    FreeBASIC RISC OS limited wide-character helpers
    ------------------------------------------------

    File: fb_unicode_riscos.h

    Purpose:

        Supply the core wide-character operations missing from UnixLib.

    Responsibilities:

        - parse the ASCII digits used by FreeBASIC numeric literals
        - support optional signs and bases from 2 through 36
        - classify and case-map the ASCII letters used by the runtime
        - format numeric values without UnixLib's aborting swprintf stub
        - replace UnixLib entry points that abort when called

    This file intentionally does NOT contain:

        - locale-sensitive Unicode digit classification
        - floating-point input parsing
        - allocation or global state
*/

#ifndef FB_UNICODE_RISCOS_H
#define FB_UNICODE_RISCOS_H

static __inline__ int fb_riscos_wstr_IsLower( FB_WCHAR c )
{
	return (c >= _LC('a')) && (c <= _LC('z'));
}

static __inline__ int fb_riscos_wstr_IsUpper( FB_WCHAR c )
{
	return (c >= _LC('A')) && (c <= _LC('Z'));
}

static __inline__ FB_WCHAR fb_riscos_wstr_ToLower( FB_WCHAR c )
{
	return fb_riscos_wstr_IsUpper( c ) ?
	       (c + (_LC('a') - _LC('A'))) : c;
}

static __inline__ FB_WCHAR fb_riscos_wstr_ToUpper( FB_WCHAR c )
{
	return fb_riscos_wstr_IsLower( c ) ?
	       (c - (_LC('a') - _LC('A'))) : c;
}

static __inline__ int fb_riscos_swprintf( FB_WCHAR *buffer, size_t chars,
	const FB_WCHAR *format, ... )
{
	char narrow_format[64];
	char *temporary;
	size_t i;
	int result;
	va_list arguments;

	if( (buffer == NULL) || (chars == 0) || (format == NULL) )
		return -1;

	buffer[0] = _LC('\0');
	for( i = 0; i < sizeof( narrow_format ) - 1; i++ ) {
		FB_WCHAR c = format[i];
		if( c == _LC('\0') )
			break;
		if( c > 127 )
			return -1;
		narrow_format[i] = (char)c;
	}

	if( format[i] != _LC('\0') )
		return -1;
	narrow_format[i] = '\0';

	temporary = (char *)malloc( chars );
	if( temporary == NULL )
		return -1;

	va_start( arguments, format );
	result = vsnprintf( temporary, chars, narrow_format, arguments );
	va_end( arguments );

	if( result >= 0 ) {
		for( i = 0; (i < chars - 1) && (temporary[i] != '\0'); i++ )
			buffer[i] = (FB_WCHAR)(unsigned char)temporary[i];
		buffer[i] = _LC('\0');
	}

	free( temporary );
	return result;
}

static __inline__ int fb_riscos_wstr_DigitValue( FB_WCHAR c )
{
	if( (c >= _LC('0')) && (c <= _LC('9')) )
		return c - _LC('0');
	if( (c >= _LC('a')) && (c <= _LC('z')) )
		return c - _LC('a') + 10;
	if( (c >= _LC('A')) && (c <= _LC('Z')) )
		return c - _LC('A') + 10;
	return -1;
}

static __inline__ unsigned long long fb_riscos_wcstoull(
	const FB_WCHAR *src, FB_WCHAR **endptr, int radix )
{
	const FB_WCHAR *p = src;
	unsigned long long value = 0;
	int negative = FALSE;
	int digit;

	if( p == NULL ) {
		if( endptr != NULL )
			*endptr = (FB_WCHAR *)src;
		return 0;
	}

	while( (*p == _LC(' ')) || (*p == _LC('\t')) )
		p++;

	if( (*p == _LC('+')) || (*p == _LC('-')) ) {
		negative = (*p == _LC('-'));
		p++;
	}

	if( radix == 0 )
		radix = 10;

	while( (digit = fb_riscos_wstr_DigitValue( *p )) >= 0 ) {
		if( digit >= radix )
			break;
		value = (value * (unsigned)radix) + (unsigned)digit;
		p++;
	}

	if( endptr != NULL )
		*endptr = (FB_WCHAR *)p;

	return negative ? (0ull - value) : value;
}

#undef wcstoul
#undef wcstoull
#undef iswlower
#undef iswupper
#undef towlower
#undef towupper
#undef swprintf
#define wcstoul(s,e,r)  ((unsigned long)fb_riscos_wcstoull( s, e, r ))
#define wcstoull(s,e,r) fb_riscos_wcstoull( s, e, r )
#define iswlower(c)      fb_riscos_wstr_IsLower( c )
#define iswupper(c)      fb_riscos_wstr_IsUpper( c )
#define towlower(c)      fb_riscos_wstr_ToLower( c )
#define towupper(c)      fb_riscos_wstr_ToUpper( c )
#define swprintf         fb_riscos_swprintf

#endif

/* end of fb_unicode_riscos.h */
