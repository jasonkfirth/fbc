/*
    FreeBASIC Runtime Library
    File: ustr_optional.c
    Purpose: Implement the Unicode paths of string.bi's text helpers.
    Responsibilities: Scalar reversal, replacement, folding, and formatting.
    This file intentionally does NOT contain compiler overload selection or
        linguistic collation. Text comparison uses Unicode default case folding.
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Unicode comparison and bounded matching                                   */
/* ------------------------------------------------------------------------- */

static FBSTRING *hFold_NoLock( FBSTRING *src )
{
	ssize_t offset = 0, bytes = 0, width, length = src ? FB_STRSIZE(src) : 0;
	char folded[12]; /* A full mapping has at most three four-byte scalars. */
	FBSTRING *result;
	while( offset < length ) {
		width = fb_hUtf8Fold( fb_hUtf8Decode( src->data, length, &offset ), folded );
		if( bytes > FB_USTRING_MAX_BYTES - width ) return NULL;
		bytes += width;
	}
	result = fb_hUStrAlloc_NoLock( bytes );
	if( result == NULL || bytes == 0 ) return result;
	offset = bytes = 0;
	while( offset < length )
		bytes += fb_hUtf8Fold( fb_hUtf8Decode( src->data, length, &offset ), result->data + bytes );
	return result;
}

static int hMatch( FBSTRING *src, ssize_t position, FBSTRING *pattern, int compare, ssize_t *end )
{
	ssize_t length = FB_STRSIZE(src), wanted = FB_STRSIZE(pattern), offset = position, matched = 0, width;
	char folded[12];
	if( wanted == 0 ) return FALSE;
	if( compare == 0 ) {
		if( wanted > length - position || memcmp(src->data + position, pattern->data, wanted) != 0 ) return FALSE;
		*end = position + wanted;
		return TRUE;
	}
	/* Require complete source scalars. Thus "ss" can match sharp s, while
	   "s" cannot select half of that scalar's two-character fold mapping. */
	while( offset < length && matched < wanted ) {
		width = fb_hUtf8Fold( fb_hUtf8Decode(src->data, length, &offset), folded );
		if( width > wanted - matched || memcmp(folded, pattern->data + matched, width) != 0 ) return FALSE;
		matched += width;
	}
	*end = offset;
	return matched == wanted;
}

FBCALL int fb_UStrComp( FBSTRING *first, FBSTRING *second, int compare )
{
	FBSTRING *a = first ? first : &__fb_ctx.null_desc, *b = second ? second : &__fb_ctx.null_desc;
	ssize_t length1, length2, common;
	int result = 0, error = FB_RTERROR_OK;
	FB_STRLOCK();
	if( compare != 0 && compare != 1 ) error = FB_RTERROR_ILLEGALFUNCTIONCALL;
	else {
		if( compare ) { a = hFold_NoLock(first); b = hFold_NoLock(second); }
		if( a == NULL || b == NULL ) error = FB_RTERROR_OUTOFMEM;
		else {
			length1 = FB_STRSIZE(a);
			length2 = FB_STRSIZE(b);
			common = length1 < length2 ? length1 : length2;
			if( common ) result = memcmp(a->data, b->data, common);
			if( result == 0 ) result = (length1 > length2) - (length1 < length2);
			else result = result > 0 ? 1 : -1;
		}
		if( compare ) fb_hUStrDeletePair_NoLock(a, b);
	}
	fb_hUStrDeletePair_NoLock(first, second);
	FB_STRUNLOCK();
	fb_ErrorSetNum(error);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Reversal and replacement                                                  */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_UStrReverse( FBSTRING *src )
{
	FBSTRING *result;
	ssize_t length = src && src->data ? FB_STRSIZE(src) : 0;
	ssize_t first, offset = 0, output = length;
	FB_STRLOCK();
	result = fb_hUStrAlloc_NoLock(length);
	if( result != NULL && length > 0 ) {
		while( offset < length ) {
			first = offset;
			fb_hUtf8Decode(src->data, length, &offset);
			output -= offset - first;
			memcpy(result->data + output, src->data + first, offset - first);
		}
	}
	fb_hStrDelTemp_NoLock(src);
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrReplace( FBSTRING *src, FBSTRING *find, FBSTRING *replacement, ssize_t start, ssize_t count, int compare )
{
	FBSTRING *result = &__fb_ctx.null_desc, *pattern = find;
	ssize_t length = src && src->data ? FB_STRSIZE(src) : 0;
	ssize_t replacement_length = replacement && replacement->data ? FB_STRSIZE(replacement) : 0;
	ssize_t offset, position, end, size, matches = 0, left, output = 0;
	int error = FB_RTERROR_OK;
	FB_STRLOCK();
	if( start < 1 || count < -1 || (compare != 0 && compare != 1) ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}
	offset = fb_hUtf8Offset(src ? src->data : NULL, length, start - 1);
	if( offset == length ) goto done;
	if( compare ) pattern = hFold_NoLock(find);
	if( pattern == NULL ) { error = FB_RTERROR_OUTOFMEM; goto done; }
	size = length - offset;
	position = offset;
	while( position < length && (count < 0 || matches < count) ) {
		if( hMatch(src, position, pattern, compare, &end) ) {
			size -= end - position;
			if( replacement_length > FB_USTRING_MAX_BYTES - size ) { error = FB_RTERROR_OUTOFMEM; goto done; }
			size += replacement_length;
			position = end;
			++matches;
		} else fb_hUtf8Decode(src->data, length, &position);
	}
	result = fb_hUStrAlloc_NoLock(size);
	if( result == NULL ) { error = FB_RTERROR_OUTOFMEM; result = &__fb_ctx.null_desc; goto done; }
	position = offset;
	left = matches;
	while( left > 0 ) {
		if( hMatch(src, position, pattern, compare, &end) ) {
			if( replacement_length ) memcpy(result->data + output, replacement->data, replacement_length);
			output += replacement_length;
			position = end;
			--left;
		} else {
			end = position;
			fb_hUtf8Decode(src->data, length, &end);
			memcpy(result->data + output, src->data + position, end - position);
			output += end - position;
			position = end;
		}
	}
	if( position < length ) memcpy(result->data + output, src->data + position, length - position);
done:
	if( compare == 1 && pattern != find ) fb_hStrDelTemp_NoLock(pattern);
	fb_hUStrDeletePair_NoLock(src, find);
	if( replacement != src && replacement != find ) fb_hStrDelTemp_NoLock(replacement);
	FB_STRUNLOCK();
	fb_ErrorSetNum(error);
	return result;
}

FBCALL FBSTRING *fb_UStrFormat( double value, FBSTRING *mask )
{
	return fb_UStrFromBytes(fb_StrFormat(value, mask), FB_STRSIZEVARLEN);
}

/* ------------------------------------------------------------------------- */
/* ABI adapters for string.bi's mixed-type overloads                          */
/* ------------------------------------------------------------------------- */

enum { TEXT_BYTES, TEXT_WIDE, TEXT_UTF8 };

static FBSTRING *hCopy_NoLock( const void *value, int kind )
{
	const FBSTRING *str = value;
	if( kind == TEXT_WIDE ) return fb_hUStrFromWstr_NoLock(value);
	if( str == NULL || str->data == NULL ) return &__fb_ctx.null_desc;
	if( kind == TEXT_BYTES ) return fb_hUStrCopy_NoLock(str->data, FB_STRSIZE(str));
	return fb_hUStrNormalize_NoLock(str->data, FB_STRSIZE(str));
}

static void hRelease_NoLock( void *a, int kind1, void *b, int kind2, void *c, int kind3 )
{
	if( kind1 != TEXT_WIDE ) fb_hStrDelTemp_NoLock(a);
	if( kind2 != TEXT_WIDE && b != a ) fb_hStrDelTemp_NoLock(b);
	if( kind3 != TEXT_WIDE && c != a && c != b ) fb_hStrDelTemp_NoLock(c);
}

static FBSTRING *hReplace( void *source, int kind1, void *find, int kind2, void *replacement, int kind3, ssize_t start, ssize_t count, int compare )
{
	FBSTRING *a, *b, *c, *result;
	FB_STRLOCK();
	a = hCopy_NoLock(source, kind1);
	b = hCopy_NoLock(find, kind1 != TEXT_BYTES && kind2 == TEXT_BYTES ? TEXT_UTF8 : kind2);
	c = hCopy_NoLock(replacement, kind1 != TEXT_BYTES && kind3 == TEXT_BYTES ? TEXT_UTF8 : kind3);
	hRelease_NoLock(source, kind1, find, kind2, replacement, kind3);
	FB_STRUNLOCK();
	if( a == NULL || b == NULL || c == NULL ) {
		fb_hStrDelTemp(a); fb_hStrDelTemp(b); fb_hStrDelTemp(c);
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return &__fb_ctx.null_desc;
	}
	result = kind1 == TEXT_BYTES ? fb_StrReplace(a, b, c, start, count, compare) : fb_UStrReplace(a, b, c, start, count, compare);
	return result;
}

static int hCompare( void *first, int kind1, void *second, int kind2, int compare )
{
	FBSTRING *a, *b;
	FB_STRLOCK();
	a = hCopy_NoLock(first, kind1 == TEXT_BYTES ? TEXT_UTF8 : kind1);
	b = hCopy_NoLock(second, kind2 == TEXT_BYTES ? TEXT_UTF8 : kind2);
	hRelease_NoLock(first, kind1, second, kind2, NULL, TEXT_WIDE);
	FB_STRUNLOCK();
	if( a == NULL || b == NULL ) {
		fb_hStrDelTemp(a); fb_hStrDelTemp(b);
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return 0;
	}
	return fb_UStrComp(a, b, compare);
}

#define TEXT_TYPE_s FBSTRING *
#define TEXT_TYPE_u FBSTRING *
#define TEXT_TYPE_w FB_WCHAR *
#define TEXT_KIND_s TEXT_BYTES
#define TEXT_KIND_u TEXT_UTF8
#define TEXT_KIND_w TEXT_WIDE
#define TEXT_RESULT_s(value) (value)
#define TEXT_RESULT_u(value) (value)
#define TEXT_RESULT_w(value) fb_UStrToWstr(value)

/* All mixed signatures are explicit so overload selection cannot tie when
   two Unicode operands have different types. Return type follows source. */
#define TEXT_REPLACE(S,F,R) \
FBCALL TEXT_TYPE_##S fb_TextReplace_##S##F##R( TEXT_TYPE_##S source, TEXT_TYPE_##F find, TEXT_TYPE_##R replacement, ssize_t start, ssize_t count, int compare ) \
{ return TEXT_RESULT_##S(hReplace(source, TEXT_KIND_##S, find, TEXT_KIND_##F, replacement, TEXT_KIND_##R, start, count, compare)); }

#define TEXT_COMPARE(A,B) \
FBCALL int fb_TextComp_##A##B( TEXT_TYPE_##A first, TEXT_TYPE_##B second, int compare ) \
{ return hCompare(first, TEXT_KIND_##A, second, TEXT_KIND_##B, compare); }

TEXT_REPLACE(s,s,u) TEXT_REPLACE(s,s,w) TEXT_REPLACE(s,u,s) TEXT_REPLACE(s,u,u) TEXT_REPLACE(s,u,w) TEXT_REPLACE(s,w,s) TEXT_REPLACE(s,w,u) TEXT_REPLACE(s,w,w)
TEXT_REPLACE(u,s,s) TEXT_REPLACE(u,s,u) TEXT_REPLACE(u,s,w) TEXT_REPLACE(u,u,s) TEXT_REPLACE(u,u,u) TEXT_REPLACE(u,u,w) TEXT_REPLACE(u,w,s) TEXT_REPLACE(u,w,u) TEXT_REPLACE(u,w,w)
TEXT_REPLACE(w,s,s) TEXT_REPLACE(w,s,u) TEXT_REPLACE(w,s,w) TEXT_REPLACE(w,u,s) TEXT_REPLACE(w,u,u) TEXT_REPLACE(w,u,w) TEXT_REPLACE(w,w,s) TEXT_REPLACE(w,w,u) TEXT_REPLACE(w,w,w)
TEXT_COMPARE(s,u) TEXT_COMPARE(s,w) TEXT_COMPARE(u,s) TEXT_COMPARE(u,u) TEXT_COMPARE(u,w) TEXT_COMPARE(w,s) TEXT_COMPARE(w,u) TEXT_COMPARE(w,w)

FBCALL FB_WCHAR *fb_TextReverse_w( FB_WCHAR *source )
{
	return fb_UStrToWstr(fb_UStrReverse(fb_UStrFromWstr(source)));
}

FBCALL FB_WCHAR *fb_TextFormat_w( double value, FB_WCHAR *mask )
{
	return fb_UStrToWstr(fb_UStrFormat(value, fb_UStrFromWstr(mask)));
}

/* end of ustr_optional.c */
