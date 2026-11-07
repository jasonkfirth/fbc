/*
 * Project: FreeBASIC runtime library
 * File: strings/str_concat.c
 * Purpose: string concatenation function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include "str_size.h"

static __inline__ void fb_hStrConcat
	(
		char *dst,
		const char *str1,
		ssize_t len1,
		const char *str2,
		ssize_t len2
	)
{
	if( len1 > 0 )
		dst = (char *) FB_MEMCPYX( dst, str1, (size_t)len1 );
	if( len2 > 0 )
		dst = (char *) FB_MEMCPYX( dst, str2, (size_t)len2 );
	*dst = '\0';
}

FBCALL FBSTRING_ACCESS *fb_StrConcat
	(
		FBSTRING_ACCESS *dst,
		void *str1,
		ssize_t str1_size,
		void *str2,
		ssize_t str2_size
	)
{
	const char *str1_ptr, *str2_ptr;
	ssize_t str1_len, str2_len, total;
	FBSTRING_ACCESS *result;

	FB_STRSETUP_FIX( str1, str1_size, str1_ptr, str1_len );

	FB_STRSETUP_FIX( str2, str2_size, str2_ptr, str2_len );

	/* NULL? */
	if( !fb_hStrAddSize( str1_len, str2_len, &total ) )
	{
		fb_StrDelete( dst );
	}
	else if( total == 0 )
	{
		fb_StrDelete( dst );
	}
	else
	{
		/* alloc temp string */
		result = fb_hStrAllocTemp( dst, total );
		if( result != NULL ) {
			dst = result;
			/* do the concatenation */
			fb_hStrConcat( dst->data, str1_ptr, str1_len, str2_ptr, str2_len );
		}
	}

	FB_STRLOCK();

	/* delete temps? */
	if( str1_size == -1 )
		fb_hStrDelTemp_NoLock( (FBSTRING_ACCESS *)str1 );
	if( str2_size == -1 )
		fb_hStrDelTemp_NoLock( (FBSTRING_ACCESS *)str2 );

	FB_STRUNLOCK();

	return dst;
}

/* end of strings/str_concat.c */
