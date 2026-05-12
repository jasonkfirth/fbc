/*
 * Xbox WSTRING helper overrides
 *
 * nxdk's wchar extension layer still leaves several C99 wide-character
 * routines as assert stubs.  FreeBASIC's core WSTRING code only needs ASCII
 * case mapping and radix parsing here, so keep those pieces local to the Xbox
 * runtime instead of scattering HOST_XBOX branches through fb_unicode.h.
 */

#ifndef FB_UNICODE_XBOX_H
#define FB_UNICODE_XBOX_H

static __inline__ int fb_xbox_wstr_IsLower( FB_WCHAR c )
{
	return (c >= _LC('a')) && (c <= _LC('z'));
}

static __inline__ int fb_xbox_wstr_IsUpper( FB_WCHAR c )
{
	return (c >= _LC('A')) && (c <= _LC('Z'));
}

static __inline__ FB_WCHAR fb_xbox_wstr_ToLower( FB_WCHAR c )
{
	return fb_xbox_wstr_IsUpper( c ) ? (c + (_LC('a') - _LC('A'))) : c;
}

static __inline__ FB_WCHAR fb_xbox_wstr_ToUpper( FB_WCHAR c )
{
	return fb_xbox_wstr_IsLower( c ) ? (c - (_LC('a') - _LC('A'))) : c;
}

static __inline__ int fb_xbox_wstr_DigitValue( FB_WCHAR c )
{
	if( (c >= _LC('0')) && (c <= _LC('9')) )
		return c - _LC('0');
	if( (c >= _LC('a')) && (c <= _LC('z')) )
		return c - _LC('a') + 10;
	if( (c >= _LC('A')) && (c <= _LC('Z')) )
		return c - _LC('A') + 10;
	return -1;
}

static __inline__ unsigned long long fb_xbox_wcstoull( const FB_WCHAR *src, FB_WCHAR **endptr, int radix )
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

	while( (digit = fb_xbox_wstr_DigitValue( *p )) >= 0 ) {
		if( digit >= radix )
			break;
		value = (value * (unsigned)radix) + (unsigned)digit;
		p++;
	}

	if( endptr != NULL )
		*endptr = (FB_WCHAR *)p;

	return negative ? (0ull - value) : value;
}

#undef iswlower
#undef iswupper
#undef towlower
#undef towupper
#undef wcstoul
#undef wcstoull
#define iswlower(c)       fb_xbox_wstr_IsLower( c )
#define iswupper(c)       fb_xbox_wstr_IsUpper( c )
#define towlower(c)       fb_xbox_wstr_ToLower( c )
#define towupper(c)       fb_xbox_wstr_ToUpper( c )
#define wcstoul(s,e,r)    ((unsigned long)fb_xbox_wcstoull( s, e, r ))
#define wcstoull(s,e,r)   fb_xbox_wcstoull( s, e, r )

#endif

/* end of xbox/fb_unicode_xbox.h */
