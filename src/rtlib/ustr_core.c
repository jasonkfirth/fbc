/*
    FreeBASIC Runtime Library
    File: ustr_core.c
    Purpose: Own UTF-8 scalar decoding and descriptor conversions.
    Responsibilities: Validate input, preserve NULs, check sizes, and manage
        temporary ownership for assignment and concatenation.
    This file intentionally does NOT contain slicing, searching, or casing.
*/

#include "fb.h"

/* ------------------------------------------------------------------------- */
/* Unicode scalar encoding                                                   */
/* ------------------------------------------------------------------------- */

unsigned int fb_hUtf8Decode( const char *data, ssize_t length, ssize_t *offset )
{
	unsigned int scalar, lead, byte, minimum, maximum;
	ssize_t position = *offset;
	int remaining, i;

	if( (data == NULL) || (position < 0) || (position >= length) )
		return 0;

	lead = (unsigned char)data[position++];
	*offset = position;
	if( lead < 0x80 )
		return lead;
	if( (lead >= 0xC2) && (lead <= 0xDF) ) {
		scalar = lead & 0x1F;
		remaining = 1;
	} else if( (lead >= 0xE0) && (lead <= 0xEF) ) {
		scalar = lead & 0x0F;
		remaining = 2;
	} else if( (lead >= 0xF0) && (lead <= 0xF4) ) {
		scalar = lead & 0x07;
		remaining = 3;
	} else {
		return FB_UTF8_REPLACEMENT;
	}

	/* Unicode 3.9.6 maximal subparts: consume only a valid prefix. The
	   second-byte bounds reject overlong forms, surrogates, and > U+10FFFF. */
	for( i = 0; i < remaining; ++i ) {
		if( position >= length )
			return FB_UTF8_REPLACEMENT;
		byte = (unsigned char)data[position];
		minimum = 0x80;
		maximum = 0xBF;
		if( i == 0 ) {
			if( lead == 0xE0 ) minimum = 0xA0;
			if( lead == 0xED ) maximum = 0x9F;
			if( lead == 0xF0 ) minimum = 0x90;
			if( lead == 0xF4 ) maximum = 0x8F;
		}
		if( (byte < minimum) || (byte > maximum) )
			return FB_UTF8_REPLACEMENT;
		scalar = (scalar << 6) | (byte & 0x3F);
		*offset = ++position;
	}
	return scalar;
}

ssize_t fb_hUtf8Encode( char *data, unsigned int scalar )
{
	if( (scalar > 0x10FFFF) || ((scalar >= 0xD800) && (scalar <= 0xDFFF)) )
		scalar = FB_UTF8_REPLACEMENT;
	if( scalar < 0x80 ) {
		data[0] = scalar;
		return 1;
	}
	if( scalar < 0x800 ) {
		data[0] = 0xC0 | (scalar >> 6);
		data[1] = 0x80 | (scalar & 0x3F);
		return 2;
	}
	if( scalar < 0x10000 ) {
		data[0] = 0xE0 | (scalar >> 12);
		data[1] = 0x80 | ((scalar >> 6) & 0x3F);
		data[2] = 0x80 | (scalar & 0x3F);
		return 3;
	}
	data[0] = 0xF0 | (scalar >> 18);
	data[1] = 0x80 | ((scalar >> 12) & 0x3F);
	data[2] = 0x80 | ((scalar >> 6) & 0x3F);
	data[3] = 0x80 | (scalar & 0x3F);
	return 4;
}

ssize_t fb_hUtf8Count( const char *data, ssize_t length )
{
	ssize_t offset = 0, count = 0;
	if( data == NULL ) return 0;
	while( offset < length ) {
		fb_hUtf8Decode( data, length, &offset );
		++count;
	}
	return count;
}

ssize_t fb_hUtf8Offset( const char *data, ssize_t length, ssize_t count )
{
	ssize_t offset = 0;
	if( data == NULL ) return 0;
	while( (offset < length) && (count > 0) ) {
		fb_hUtf8Decode( data, length, &offset );
		--count;
	}
	return offset;
}

/* ------------------------------------------------------------------------- */
/* Descriptor storage and ownership                                          */
/* ------------------------------------------------------------------------- */

FBSTRING *fb_hUStrAlloc_NoLock( ssize_t length )
{
	FBSTRING *result;
	if( length == 0 ) return &__fb_ctx.null_desc;
	if( (length < 0) || (length > FB_USTRING_MAX_BYTES) ) {
		fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		return NULL;
	}
	result = fb_hStrAllocTemp_NoLock( NULL, length );
	if( result == NULL ) fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	else result->data[length] = 0;
	return result;
}

FBSTRING *fb_hUStrCopy_NoLock( const char *data, ssize_t length )
{
	FBSTRING *result = fb_hUStrAlloc_NoLock( length );
	if( (result != NULL) && (length > 0) )
		memcpy( result->data, data, length );
	return result;
}

FBSTRING *fb_hUStrNormalize_NoLock( const char *data, ssize_t length )
{
	ssize_t offset = 0, bytes = 0, width;
	char encoded[4];
	FBSTRING *result;
	if( data == NULL ) return &__fb_ctx.null_desc;
	while( offset < length ) {
		width = fb_hUtf8Encode( encoded, fb_hUtf8Decode( data, length, &offset ) );
		if( bytes > FB_USTRING_MAX_BYTES - width ) {
			fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			return NULL;
		}
		bytes += width;
	}
	result = fb_hUStrAlloc_NoLock( bytes );
	if( (result == NULL) || (bytes == 0) ) return result;
	offset = bytes = 0;
	while( offset < length )
		bytes += fb_hUtf8Encode( result->data + bytes,
			fb_hUtf8Decode( data, length, &offset ) );
	return result;
}

void fb_hUStrDeletePair_NoLock( FBSTRING *first, FBSTRING *second )
{
	fb_hStrDelTemp_NoLock( first );
	if( second != first ) fb_hStrDelTemp_NoLock( second );
}

void fb_hUStrMove_NoLock( FBSTRING *dst, FBSTRING *src, int is_init )
{
	ssize_t flags;
	/* Build the replacement before releasing the old buffer. This makes
	   self-assignment and aliased MID sources safe, including failed allocs. */
	if( src == NULL ) return;
	flags = is_init ? 0 : dst->len & FB_TEMPSTRBIT;
	if( !is_init ) fb_StrDelete( dst );
	dst->data = src->data;
	dst->len = FB_STRSIZE( src ) | flags;
	dst->size = src->size;
	if( src != &__fb_ctx.null_desc ) {
		src->data = NULL;
		fb_hStrDelTemp_NoLock( src );
	}
}

FBCALL FBSTRING *fb_UStrToStr( FBSTRING *src )
{
	FBSTRING *result = &__fb_ctx.null_desc;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) )
		result = fb_hUStrCopy_NoLock( src->data, FB_STRSIZE(src) );
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FBSTRING *fb_UStrFromBytes( void *src, ssize_t size )
{
	const char *data;
	ssize_t length;
	FBSTRING *result;
	FB_STRLOCK();
	FB_STRSETUP_FIX( src, size, data, length );
	result = fb_hUStrNormalize_NoLock( data, length );
	if( size == FB_STRSIZEVARLEN ) fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

static void *hAssign( void *dst, void *src, ssize_t size, int is_init )
{
	const char *data;
	ssize_t length;
	FBSTRING *result;
	FB_STRLOCK();
	FB_STRSETUP_FIX( src, size, data, length );
	result = fb_hUStrNormalize_NoLock( data, length );
	if( dst != NULL ) {
		if( (result == NULL) && is_init ) memset( dst, 0, sizeof(FBSTRING) );
		else fb_hUStrMove_NoLock( dst, result, is_init );
	} else {
		fb_hStrDelTemp_NoLock( result );
	}
	if( (size == FB_STRSIZEVARLEN) && (src != dst) )
		fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return dst;
}

FBCALL void *fb_UStrAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem )
{
	return hAssign( dst, src, src_size, FB_FALSE );
}

FBCALL void *fb_UStrInit( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem )
{
	return hAssign( dst, src, src_size, FB_TRUE );
}

FBCALL ssize_t fb_UStrLen( void *src, ssize_t size )
{
	const char *data;
	ssize_t length, result;
	FB_STRLOCK();
	FB_STRSETUP_FIX( src, size, data, length );
	result = fb_hUtf8Count( data, length );
	if( size == FB_STRSIZEVARLEN ) fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result;
}

FBCALL unsigned int fb_UStrAsc( FBSTRING *src, ssize_t position )
{
	ssize_t offset, length;
	unsigned int result = 0;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) && (position > 0) ) {
		length = FB_STRSIZE(src);
		offset = fb_hUtf8Offset( src->data, length, position - 1 );
		if( offset < length ) result = fb_hUtf8Decode( src->data, length, &offset );
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result;
}

FBCALL unsigned int fb_UStrIndex( FBSTRING *src, ssize_t index )
{
	/* Avoid overflowing the one-based ASC position for a maximal index. */
	if( (index < 0) || (index >= FB_USTRING_MAX_BYTES) ) {
		fb_hStrDelTemp( src );
		return 0;
	}
	return fb_UStrAsc( src, index + 1 );
}

/* ------------------------------------------------------------------------- */
/* Concatenation and wide conversion                                         */
/* ------------------------------------------------------------------------- */

static FBSTRING *hConcat( void *first, ssize_t first_size, void *second, ssize_t second_size )
{
	const char *data;
	ssize_t length, first_length, second_length;
	FBSTRING *a, *b, *result = NULL;
	FB_STRLOCK();
	FB_STRSETUP_FIX( first, first_size, data, length );
	a = fb_hUStrNormalize_NoLock( data, length );
	FB_STRSETUP_FIX( second, second_size, data, length );
	b = fb_hUStrNormalize_NoLock( data, length );
	if( (a != NULL) && (b != NULL) ) {
		first_length = FB_STRSIZE(a);
		second_length = FB_STRSIZE(b);
		if( first_length <= FB_USTRING_MAX_BYTES - second_length ) {
			result = fb_hUStrAlloc_NoLock( first_length + second_length );
			if( (result != NULL) && (first_length + second_length > 0) ) {
				if( first_length ) memcpy( result->data, a->data, first_length );
				if( second_length ) memcpy( result->data + first_length, b->data, second_length );
			}
		} else fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	fb_hUStrDeletePair_NoLock( a, b );
	if( first_size == FB_STRSIZEVARLEN ) fb_hStrDelTemp_NoLock( first );
	if( (second_size == FB_STRSIZEVARLEN) && (second != first) ) fb_hStrDelTemp_NoLock( second );
	FB_STRUNLOCK();
	return result;
}

FBCALL FBSTRING *fb_UStrConcat( FBSTRING *dst, void *first, ssize_t first_size, void *second, ssize_t second_size )
{
	FBSTRING *result = hConcat( first, first_size, second, second_size );
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL void *fb_UStrConcatAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill_rem )
{
	FBSTRING view = { NULL, 0, 0 };
	FBSTRING *result;
	if( dst == NULL ) {
		if( src_size == FB_STRSIZEVARLEN ) fb_hStrDelTemp( src );
		return NULL;
	}
	if( dst != NULL ) {
		view.data = ((FBSTRING *)dst)->data;
		view.len = FB_STRSIZE(dst);
		view.size = ((FBSTRING *)dst)->size;
	}
	/* The destination is borrowed, even if its descriptor is temporary.
	   hConcat consumes source temporaries, so self-concatenation must use
	   the same borrowed view for both operands. */
	result = hConcat( &view, FB_STRSIZEVARLEN, src == dst ? &view : src, src_size );
	if( (dst != NULL) && (result != NULL) ) {
		FB_STRLOCK();
		fb_hUStrMove_NoLock( dst, result, FB_FALSE );
		FB_STRUNLOCK();
	}
	return dst;
}

static unsigned int hWideDecode( const FB_WCHAR *src, ssize_t *offset )
{
	unsigned int scalar, low;
	if( sizeof(FB_WCHAR) == 1 ) scalar = (unsigned char)src[(*offset)++];
	else scalar = src[(*offset)++];
	if( sizeof(FB_WCHAR) == 2 ) {
		if( (scalar >= 0xD800) && (scalar <= 0xDBFF) ) {
			low = src[*offset];
			if( (low >= 0xDC00) && (low <= 0xDFFF) ) {
				++*offset;
				scalar = 0x10000 + ((scalar - 0xD800) << 10) + low - 0xDC00;
			} else scalar = FB_UTF8_REPLACEMENT;
		}
	}
	return scalar;
}

FBSTRING *fb_hUStrFromWstr_NoLock( const FB_WCHAR *src )
{
	ssize_t offset = 0, bytes = 0, width;
	char encoded[4];
	FBSTRING *result = NULL;
	if( src == NULL ) result = &__fb_ctx.null_desc;
	else {
		while( src[offset] ) {
			width = fb_hUtf8Encode( encoded, hWideDecode( src, &offset ) );
			if( bytes > FB_USTRING_MAX_BYTES - width ) break;
			bytes += width;
		}
		if( src[offset] ) fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		else result = fb_hUStrAlloc_NoLock( bytes );
		if( (result != NULL) && (bytes > 0) ) {
			offset = bytes = 0;
			while( src[offset] )
				bytes += fb_hUtf8Encode( result->data + bytes, hWideDecode( src, &offset ) );
		}
	}
	return result;
}

FBCALL FBSTRING *fb_UStrFromWstr( const FB_WCHAR *src )
{
	FBSTRING *result;
	FB_STRLOCK();
	result = fb_hUStrFromWstr_NoLock( src );
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL FB_WCHAR *fb_UStrToWstr( FBSTRING *src )
{
	ssize_t offset = 0, length, units = 0;
	unsigned int scalar;
	FB_WCHAR *result = NULL;
	FB_STRLOCK();
	if( (src != NULL) && (src->data != NULL) ) {
		length = FB_STRSIZE(src);
		while( offset < length ) {
			scalar = fb_hUtf8Decode( src->data, length, &offset );
			units += (sizeof(FB_WCHAR) == 2 && scalar > 0xFFFF) ? 2 : 1;
		}
		if( units <= (FB_USTRING_MAX_BYTES / (ssize_t)sizeof(FB_WCHAR)) - 1 )
			result = fb_wstr_AllocTemp( units );
		if( result == NULL ) fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		else {
			offset = units = 0;
			while( offset < length ) {
				scalar = fb_hUtf8Decode( src->data, length, &offset );
				if( (sizeof(FB_WCHAR) == 2) && (scalar > 0xFFFF) ) {
					scalar -= 0x10000;
					result[units++] = 0xD800 + (scalar >> 10);
					result[units++] = 0xDC00 + (scalar & 0x3FF);
				} else {
					/* Byte-wide targets cannot represent non-Latin-1 scalars. */
					if( (sizeof(FB_WCHAR) == 1) && (scalar > 0xFF) ) scalar = '?';
					result[units++] = scalar;
				}
			}
			result[units] = 0;
		}
	}
	fb_hStrDelTemp_NoLock( src );
	FB_STRUNLOCK();
	return result;
}

/* end of ustr_core.c */
