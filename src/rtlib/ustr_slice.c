/*
    FreeBASIC Runtime Library
    File: ustr_slice.c
    Purpose: Implement UTF-8 slicing, repetition, and bounded text replacement.
    Responsibilities: Translate scalar positions to bytes and preserve ownership.
    This file intentionally does NOT contain search or case conversion.
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Slices and scalar construction                                             */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_UStrMid( FBSTRING *src, ssize_t start, ssize_t count )
{
	ssize_t length, first, last;
	FBSTRING *result = &__fb_ctx.null_desc;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) && (start > 0) && (count != 0) ) {
		length = FB_STRSIZE(src);
		first = fb_hUtf8Offset( src->data, length, start - 1 );
		last = count < 0 ? length : first + fb_hUtf8Offset(
			src->data + first, length - first, count );
		result = fb_hUStrCopy_NoLock( src->data + first, last - first );
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrLeft( FBSTRING *src, ssize_t count )
{
	return fb_UStrMid( src, 1, count < 0 ? 0 : count );
}

FBCALL void fb_UStrLeftSelf( FBSTRING *dst, ssize_t count )
{
	ssize_t length;
	if( dst == NULL ) return;
	FB_STRLOCK();
	if( count >= 0 ) {
		length = fb_hUtf8Offset(dst->data, FB_STRSIZE(dst), count);
		fb_hStrSetLength(dst, length);
		if( dst->data != NULL ) dst->data[length] = 0;
	}
	/* Match LEFTSELF's existing temporary-descriptor ownership contract. */
	fb_hStrDelTemp_NoLock(dst);
	FB_STRUNLOCK();
}

FBCALL void fb_WstrLeftSelf( FB_WCHAR *dst, ssize_t count )
{
	ssize_t offset = 0, remaining = count;
	if( dst == NULL || count < 0 ) return;
	/* Truncation cannot extend the caller's buffer. Treat a UTF-16 pair
	   as one scalar so the new terminator never separates the pair. */
	while( remaining > 0 && dst[offset] != 0 ) {
		if( sizeof(FB_WCHAR) == 2 && dst[offset] >= 0xD800 && dst[offset] <= 0xDBFF &&
		    dst[offset + 1] >= 0xDC00 && dst[offset + 1] <= 0xDFFF ) ++offset;
		++offset;
		--remaining;
	}
	dst[offset] = 0;
}

FBCALL FBSTRING *fb_UStrRight( FBSTRING *src, ssize_t count )
{
	ssize_t length, scalars, first;
	FBSTRING *result = &__fb_ctx.null_desc;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) && (count > 0) ) {
		length = FB_STRSIZE(src);
		scalars = fb_hUtf8Count( src->data, length );
		first = fb_hUtf8Offset( src->data, length, count < scalars ? scalars - count : 0 );
		result = fb_hUStrCopy_NoLock( src->data + first, length - first );
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBSTRING *fb_UStrChr( int count, ... )
{
	va_list args;
	ssize_t bytes = 0, scalar;
	FBSTRING *result = &__fb_ctx.null_desc;
	int i;
	FB_STRLOCK();
	/* The parser permits at most 32 arguments, matching CHR/WCHR. */
	if( (count > 0) && (count <= 32) ) {
		result = fb_hUStrAlloc_NoLock( (ssize_t)count * 4 );
		if( result != NULL ) {
			va_start( args, count );
			for( i = 0; i < count; ++i ) {
				scalar = va_arg( args, ssize_t );
				if( (scalar < 0) || (scalar > 0x10FFFF) ) scalar = FB_UTF8_REPLACEMENT;
				bytes += fb_hUtf8Encode( result->data + bytes, (unsigned int)scalar );
			}
			va_end( args );
			fb_hStrSetLength( result, bytes );
			result->data[bytes] = 0;
		}
	}
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

static FBSTRING *hFill_NoLock( ssize_t count, unsigned int scalar )
{
	char encoded[4];
	ssize_t width, offset;
	FBSTRING *result;
	if( count <= 0 ) return &__fb_ctx.null_desc;
	width = fb_hUtf8Encode( encoded, scalar );
	if( count > FB_USTRING_MAX_BYTES / width ) {
		fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		return NULL;
	}
	result = fb_hUStrAlloc_NoLock( count * width );
	if( result != NULL ) {
		for( offset = 0; offset < count * width; offset += width )
			memcpy( result->data + offset, encoded, width );
	}
	return result;
}

FBCALL FBSTRING *fb_UStrFill1( ssize_t count, ssize_t scalar )
{
	FBSTRING *result;
	FB_STRLOCK();
	if( (scalar < 0) || (scalar > 0x10FFFF) ) scalar = FB_UTF8_REPLACEMENT;
	result = hFill_NoLock( count, (unsigned int)scalar );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrFill2( ssize_t count, FBSTRING *src )
{
	ssize_t offset = 0;
	FBSTRING *result = &__fb_ctx.null_desc;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) && (FB_STRSIZE(src) > 0) )
		result = hFill_NoLock( count, fb_hUtf8Decode( src->data, FB_STRSIZE(src), &offset ) );
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

/* ------------------------------------------------------------------------- */
/* Replacement and alignment                                                 */
/* ------------------------------------------------------------------------- */

FBCALL void fb_UStrAssignMid( FBSTRING *dst, ssize_t start, ssize_t count, FBSTRING *src )
{
	ssize_t first, last, copied, replaced = 0, src_offset = 0;
	ssize_t dst_length, src_length;
	FBSTRING *result;
	FB_STRLOCK();
	if( (dst != NULL) && (dst->data != NULL) && (src != NULL) &&
	    (src->data != NULL) && (start > 0) && (count != 0) ) {
		dst_length = FB_STRSIZE(dst);
		src_length = FB_STRSIZE(src);
		first = last = fb_hUtf8Offset( dst->data, dst_length, start - 1 );
		/* Count the replacement by scalars, not encoded widths. Both the
		   source and the destination limit the number of replaced scalars. */
		while( (last < dst_length) && (src_offset < src_length) &&
		       ((count < 0) || (replaced < count)) ) {
			fb_hUtf8Decode( dst->data, dst_length, &last );
			fb_hUtf8Decode( src->data, src_length, &src_offset );
			++replaced;
		}
		copied = src_offset;
		if( dst_length - (last - first) <= FB_USTRING_MAX_BYTES - copied ) {
			result = fb_hUStrAlloc_NoLock( dst_length - (last - first) + copied );
			if( result != NULL ) {
				if( first ) memcpy( result->data, dst->data, first );
				if( copied ) memcpy( result->data + first, src->data, copied );
				if( dst_length > last ) memcpy( result->data + first + copied, dst->data + last, dst_length - last );
				fb_hUStrMove_NoLock( dst, result, FB_FALSE );
			}
		} else fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	if( src != dst ) fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
}

static void hAlign( FBSTRING *dst, FBSTRING *src, int right )
{
	ssize_t capacity, count, copied, padding;
	FBSTRING *result;
	FB_STRLOCK();
	if( dst != NULL ) {
		capacity = fb_hUtf8Count( dst->data, FB_STRSIZE(dst) );
		count = src ? fb_hUtf8Count( src->data, FB_STRSIZE(src) ) : 0;
		if( count > capacity ) count = capacity;
		copied = src ? fb_hUtf8Offset( src->data, FB_STRSIZE(src), count ) : 0;
		padding = capacity - count;
		if( copied <= FB_USTRING_MAX_BYTES - padding ) {
			result = fb_hUStrAlloc_NoLock( copied + padding );
			if( (result != NULL) && (copied + padding > 0) ) {
				if( right ) {
					memset( result->data, ' ', padding );
					if( copied ) memcpy( result->data + padding, src->data, copied );
				} else {
					if( copied ) memcpy( result->data, src->data, copied );
					memset( result->data + copied, ' ', padding );
				}
			}
			fb_hUStrMove_NoLock( dst, result, FB_FALSE );
		} else fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	if( src != dst ) fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
}

FBCALL void fb_UStrLset( FBSTRING *dst, FBSTRING *src )
{
	hAlign( dst, src, FB_FALSE );
}

FBCALL void fb_UStrSetIndex( FBSTRING *dst, ssize_t index, ssize_t scalar )
{
	char encoded[4];
	FBSTRING source;
	if( (dst == NULL) || (index < 0) ||
	    (index >= fb_hUtf8Count( dst->data, FB_STRSIZE(dst) )) ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return;
	}
	if( (scalar < 0) || (scalar > 0x10FFFF) ) scalar = FB_UTF8_REPLACEMENT;
	source.data = encoded;
	source.len = source.size = fb_hUtf8Encode( encoded, scalar );
	fb_UStrAssignMid( dst, index + 1, 1, &source );
}

FBCALL void fb_UStrRset( FBSTRING *dst, FBSTRING *src )
{
	hAlign( dst, src, FB_TRUE );
}

/* end of ustr_slice.c */
