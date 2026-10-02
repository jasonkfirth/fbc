/*
    FreeBASIC Runtime Library
    File: ustr_profile.c
    Purpose: Preserve Unicode at the profiler's terminated-byte interfaces.
    Responsibilities: Transcode wide names and bound wide/UTF-8 output copies.
    This file intentionally does NOT contain profiler state or report writers.
*/

#include "fb.h"
#include "fb_profile.h"

/* Conversion owns a temporary descriptor. Release the string lock before
   entering the profiler, whose state has a separate lock. */
static FBSTRING *hWideName( const FB_WCHAR *name )
{
	FBSTRING *text;
	FB_STRLOCK();
	text = fb_hUStrFromWstr_NoLock(name);
	FB_STRUNLOCK();
	return text;
}

#define WIDE_PROFILE(name, result_type, operation, failure) \
FBCALL result_type name( const FB_WCHAR *value ) \
{ \
	FBSTRING *text = hWideName(value); \
	result_type result; \
	if( text == NULL ) return failure; \
	result = operation(text->data ? text->data : ""); \
	fb_hStrDelTemp(text); \
	return result; \
}

WIDE_PROFILE(fb_WstrProfileBeginProc, void *, fb_ProfileBeginProc, NULL)
WIDE_PROFILE(fb_WstrProfileBeginCall, void *, fb_ProfileBeginCall, NULL)
WIDE_PROFILE(fb_WstrProfileSetFileName, int, fb_ProfileSetFileName, FB_RTERROR_OUTOFMEM)

FBCALL void fb_WstrProfileIgnore( const FB_WCHAR *value )
{
	FBSTRING *text = hWideName(value);
	if( text == NULL ) return;
	fb_ProfileIgnore(text->data ? text->data : "");
	fb_hStrDelTemp(text);
}

FBCALL int fb_WstrProfileGetFileName( FB_WCHAR *dst, int capacity )
{
	char buffer[PROFILER_MAX_PATH];
	FBSTRING borrowed;
	FB_WCHAR *wide;
	ssize_t units;
	int result;
	if( dst == NULL || capacity <= 0 )
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	result = fb_ProfileGetFileName(buffer, sizeof(buffer));
	if( result != FB_RTERROR_OK ) return result;
	borrowed.data = buffer;
	borrowed.len = borrowed.size = strlen(buffer);
	wide = fb_UStrToWstr(&borrowed);
	if( wide == NULL ) return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
	units = 0;
	while( wide[units] ) ++units;
	if( units >= capacity ) {
		units = capacity - 1;
		/* A bounded UTF-16 output must never end on half of a pair. */
		if( sizeof(FB_WCHAR) == 2 && units > 0 &&
		    wide[units - 1] >= 0xD800 && wide[units - 1] <= 0xDBFF ) --units;
	}
	memcpy(dst, wide, units * sizeof(FB_WCHAR));
	dst[units] = 0;
	free(wide);
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL int fb_UStrProfileGetFileName( FBSTRING *dst, int capacity )
{
	char buffer[PROFILER_MAX_PATH];
	ssize_t bytes, boundary = 0, next;
	FBSTRING *text;
	int result;
	if( dst == NULL || capacity <= 0 )
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	result = fb_ProfileGetFileName(buffer, sizeof(buffer));
	if( result != FB_RTERROR_OK ) return result;
	FB_STRLOCK();
	text = fb_hUStrNormalize_NoLock(buffer, strlen(buffer));
	result = text ? FB_RTERROR_OK : FB_RTERROR_OUTOFMEM;
	if( text != NULL ) {
		/* Apply the byte limit after normalization, which can expand malformed
		   byte names supplied through the legacy C interface. */
		bytes = FB_STRSIZE(text);
		while( boundary < bytes ) {
			next = boundary;
			fb_hUtf8Decode(text->data, bytes, &next);
			if( next >= capacity ) break;
			boundary = next;
		}
		fb_hStrSetLength(text, boundary);
		if( text->data ) text->data[boundary] = 0;
		fb_hUStrMove_NoLock(dst, text, FB_FALSE);
	}
	FB_STRUNLOCK();
	return fb_ErrorSetNum(result);
}

/* end of ustr_profile.c */
