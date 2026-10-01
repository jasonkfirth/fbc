/*
    FreeBASIC Runtime Library
    File: ustr_case.c
    Purpose: Apply Unicode 17.0 default full upper/lower case conversion.
    Responsibilities: Handle expansions and contextual Greek final sigma.
    This file intentionally does NOT contain locale tailoring or normalization.
*/

#include "fb.h"

typedef struct {
	unsigned int scalar;
	unsigned int count;
	unsigned int mapped[3];
} UTF8_CASE_MAP;

typedef struct {
	unsigned int first, last;
} UTF8_CASE_RANGE;

#include "ustr_case_data.h"

/* ------------------------------------------------------------------------- */
/* Unicode mapping and context                                               */
/* ------------------------------------------------------------------------- */

static int hProperty( unsigned int scalar, const UTF8_CASE_RANGE *ranges, size_t count )
{
	size_t first = 0, middle;
	while( first < count ) {
		middle = first + (count - first) / 2;
		if( scalar < ranges[middle].first ) count = middle;
		else if( scalar > ranges[middle].last ) first = middle + 1;
		else return FB_TRUE;
	}
	return FB_FALSE;
}

static const UTF8_CASE_MAP *hMapping( unsigned int scalar, int lower )
{
	const UTF8_CASE_MAP *map = lower ? lower_map : upper_map;
	size_t first = 0, middle, count = lower ? sizeof(lower_map) / sizeof(*map) : sizeof(upper_map) / sizeof(*map);
	while( first < count ) {
		middle = first + (count - first) / 2;
		if( scalar < map[middle].scalar ) count = middle;
		else if( scalar > map[middle].scalar ) first = middle + 1;
		else return &map[middle];
	}
	return NULL;
}

ssize_t fb_hUtf8Fold( unsigned int scalar, char *output )
{
	size_t first = 0, count = sizeof(fold_map) / sizeof(*fold_map), middle;
	const UTF8_CASE_MAP *mapping = NULL;
	ssize_t bytes = 0;
	unsigned int i;
	while( first < count ) {
		middle = first + (count - first) / 2;
		if( scalar < fold_map[middle].scalar ) count = middle;
		else if( scalar > fold_map[middle].scalar ) first = middle + 1;
		else { mapping = &fold_map[middle]; break; }
	}
	if( mapping == NULL ) return fb_hUtf8Encode( output, scalar );
	for( i = 0; i < mapping->count; ++i )
		bytes += fb_hUtf8Encode( output + bytes, mapping->mapped[i] );
	return bytes;
}

static int hCased( unsigned int scalar )
{
	return hProperty( scalar, cased_ranges, sizeof(cased_ranges) / sizeof(*cased_ranges) );
}

static int hIgnorable( unsigned int scalar )
{
	return hProperty( scalar, case_ignorable_ranges, sizeof(case_ignorable_ranges) / sizeof(*case_ignorable_ranges) );
}

static int hFinalSigma( const char *data, ssize_t length, ssize_t offset, int preceding_cased )
{
	unsigned int scalar;
	if( !preceding_cased ) return FB_FALSE;
	while( offset < length ) {
		scalar = fb_hUtf8Decode( data, length, &offset );
		if( !hIgnorable( scalar ) ) return !hCased( scalar );
	}
	return FB_TRUE;
}

/* One pass computes the exact allocation size; the next writes the same
   mapping. The context always observes the original text. Mode 1 is the
   existing ASCII-only option and leaves every non-ASCII scalar intact. */
static ssize_t hConvert( const char *data, ssize_t length, char *output, int mode, int lower )
{
	ssize_t offset = 0, bytes = 0, width;
	unsigned int scalar, mapped[3], count, i;
	const UTF8_CASE_MAP *mapping;
	char encoded[4];
	int preceding_cased = FB_FALSE;
	while( offset < length ) {
		scalar = fb_hUtf8Decode( data, length, &offset );
		mapped[0] = scalar;
		count = 1;
		if( mode == 1 ) {
			if( lower && (scalar >= 'A') && (scalar <= 'Z') ) mapped[0] += 'a' - 'A';
			if( !lower && (scalar >= 'a') && (scalar <= 'z') ) mapped[0] -= 'a' - 'A';
		} else if( lower && (scalar == 0x03A3) && hFinalSigma( data, length, offset, preceding_cased ) ) {
			mapped[0] = 0x03C2;
		} else {
			mapping = hMapping( scalar, lower );
			if( mapping != NULL ) {
				count = mapping->count;
				for( i = 0; i < count; ++i ) mapped[i] = mapping->mapped[i];
			}
		}
		for( i = 0; i < count; ++i ) {
			width = fb_hUtf8Encode( encoded, mapped[i] );
			if( bytes > FB_USTRING_MAX_BYTES - width ) return -1;
			if( output != NULL ) memcpy( output + bytes, encoded, width );
			bytes += width;
		}
		if( !hIgnorable( scalar ) ) preceding_cased = hCased( scalar );
	}
	return bytes;
}

static FBSTRING *hCase( FBSTRING *src, int mode, int lower )
{
	FBSTRING *result = &__fb_ctx.null_desc;
	ssize_t bytes, length;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) ) {
		length = FB_STRSIZE(src);
		bytes = hConvert( src->data, length, NULL, mode, lower );
		result = fb_hUStrAlloc_NoLock( bytes );
		if( (result != NULL) && (bytes > 0) )
			hConvert( src->data, length, result->data, mode, lower );
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrUcase( FBSTRING *src, int mode )
{
	return hCase( src, mode, FB_FALSE );
}

FBCALL FBSTRING *fb_UStrLcase( FBSTRING *src, int mode )
{
	return hCase( src, mode, FB_TRUE );
}

/* end of ustr_case.c */
