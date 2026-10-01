/*
    FreeBASIC Runtime Library
    File: ustr_search.c
    Purpose: Search and trim UTF-8 strings at Unicode scalar boundaries.
    Responsibilities: Implement INSTR/INSTRREV and exact/ANY trimming.
    This file intentionally does NOT contain locale-dependent collation.
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Scalar membership and substring search                                    */
/* ------------------------------------------------------------------------- */

static int hContains( FBSTRING *pattern, unsigned int scalar )
{
	ssize_t offset = 0, length;
	if( (pattern == NULL) || (pattern->data == NULL) ) return FB_FALSE;
	length = FB_STRSIZE(pattern);
	while( offset < length ) {
		if( fb_hUtf8Decode( pattern->data, length, &offset ) == scalar )
			return FB_TRUE;
	}
	return FB_FALSE;
}

static ssize_t hFind( FBSTRING *src, FBSTRING *pattern, ssize_t start, int any, int reverse )
{
	ssize_t length, pattern_length, count, offset = 0, next, position = 1, result = 0;
	unsigned int scalar;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) && (pattern != NULL) && (pattern->data != NULL) ) {
		length = FB_STRSIZE(src);
		pattern_length = FB_STRSIZE(pattern);
		count = fb_hUtf8Count( src->data, length );
		if( reverse && (start < 0) ) start = count;
		if( (start > 0) && (start <= count) && (pattern_length > 0) ) {
			if( !reverse ) {
				offset = fb_hUtf8Offset( src->data, length, start - 1 );
				position = start;
			}
			while( offset < length ) {
				if( reverse && (position > start) ) break;
				next = offset;
				scalar = fb_hUtf8Decode( src->data, length, &next );
				if( any ? hContains( pattern, scalar ) :
				    ((pattern_length <= length - offset) &&
				     (memcmp( src->data + offset, pattern->data, pattern_length ) == 0)) ) {
					result = position;
					if( !reverse ) break;
				}
				offset = next;
				++position;
			}
		}
	}
	fb_hUStrDeletePair_NoLock( src, pattern );
	FB_STRUNLOCK();
	return result;
}

FBCALL ssize_t fb_UStrInstr( ssize_t start, FBSTRING *src, FBSTRING *pattern )
{
	return hFind( src, pattern, start, FB_FALSE, FB_FALSE );
}

FBCALL ssize_t fb_UStrInstrAny( ssize_t start, FBSTRING *src, FBSTRING *pattern )
{
	return hFind( src, pattern, start, FB_TRUE, FB_FALSE );
}

FBCALL ssize_t fb_UStrInstrRev( FBSTRING *src, FBSTRING *pattern, ssize_t start )
{
	return hFind( src, pattern, start, FB_FALSE, FB_TRUE );
}

FBCALL ssize_t fb_UStrInstrRevAny( FBSTRING *src, FBSTRING *pattern, ssize_t start )
{
	return hFind( src, pattern, start, FB_TRUE, FB_TRUE );
}

/* ------------------------------------------------------------------------- */
/* Trimming                                                                  */
/* ------------------------------------------------------------------------- */

static FBSTRING *hTrim( FBSTRING *src, FBSTRING *pattern, int any, int left, int right )
{
	ssize_t length, pattern_length, first = 0, last, next, previous, offset;
	FBSTRING *result = &__fb_ctx.null_desc;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) ) {
		length = last = FB_STRSIZE(src);
		pattern_length = pattern ? FB_STRSIZE(pattern) : 0;
		if( left ) {
			while( first < last ) {
				next = first;
				if( any ? hContains( pattern, fb_hUtf8Decode( src->data, length, &next ) ) :
				    (pattern == NULL ? src->data[first] == ' ' :
				     ((pattern_length > 0) && (pattern_length <= last - first) &&
				      (memcmp( src->data + first, pattern->data, pattern_length ) == 0))) ) {
					first = any ? next : first + (pattern ? pattern_length : 1);
				} else break;
			}
		}
		if( right ) {
			while( last > first ) {
				if( any ) {
					/* USTRING is valid UTF-8, so continuation bytes identify
					   the preceding scalar without scanning the whole prefix. */
					previous = last - 1;
					while( (previous > first) &&
					       (((unsigned char)src->data[previous] & 0xC0) == 0x80) )
						--previous;
					offset = previous;
					if( !hContains( pattern, fb_hUtf8Decode( src->data, last, &offset ) ) ) break;
					last = previous;
				} else if( pattern == NULL ) {
					if( src->data[last - 1] != ' ' ) break;
					--last;
				} else {
					if( (pattern_length <= 0) || (pattern_length > last - first) ||
					    memcmp( src->data + last - pattern_length, pattern->data, pattern_length ) ) break;
					last -= pattern_length;
				}
			}
		}
		result = fb_hUStrCopy_NoLock( src->data + first, last - first );
	}
	fb_hUStrDeletePair_NoLock( src, pattern );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrTrim( FBSTRING *src )
{
	return hTrim( src, NULL, FB_FALSE, FB_TRUE, FB_TRUE );
}

FBCALL FBSTRING *fb_UStrTrimEx( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_FALSE, FB_TRUE, FB_TRUE );
}

FBCALL FBSTRING *fb_UStrTrimAny( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_TRUE, FB_TRUE, FB_TRUE );
}

FBCALL FBSTRING *fb_UStrLTrim( FBSTRING *src )
{
	return hTrim( src, NULL, FB_FALSE, FB_TRUE, FB_FALSE );
}

FBCALL FBSTRING *fb_UStrLTrimEx( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_FALSE, FB_TRUE, FB_FALSE );
}

FBCALL FBSTRING *fb_UStrLTrimAny( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_TRUE, FB_TRUE, FB_FALSE );
}

FBCALL FBSTRING *fb_UStrRTrim( FBSTRING *src )
{
	return hTrim( src, NULL, FB_FALSE, FB_FALSE, FB_TRUE );
}

FBCALL FBSTRING *fb_UStrRTrimEx( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_FALSE, FB_FALSE, FB_TRUE );
}

FBCALL FBSTRING *fb_UStrRTrimAny( FBSTRING *src, FBSTRING *pattern )
{
	return hTrim( src, pattern, FB_TRUE, FB_FALSE, FB_TRUE );
}

/* end of ustr_search.c */
