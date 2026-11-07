/*
 * Project: FreeBASIC runtime library
 * File: strings/str_concatassign.c
 * Purpose: string concat and assigning (s = s + expr) function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include "str_size.h"

FBCALL void *fb_StrConcatAssign
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size,
		int fillrem
	)
{
	FBSTRING_ACCESS *dstr;
	const char *src_ptr;
	ssize_t src_len, dst_len, total;

	if( dst == NULL )
	{
		/* delete temp? */
		if( src_size == FB_STRSIZEVARLEN )
			fb_hStrDelTemp( (FBSTRING_ACCESS *)src );

		return dst;
	}

	/* src */
	FB_STRSETUP_FIX( src, src_size, src_ptr, src_len );

	/* not NULL? */
	if( src_len > 0 )
	{
		/* is dst var-len? */
		if( dst_size == FB_STRSIZEVARLEN )
		{
			dstr = (FBSTRING_ACCESS *)dst;
			dst_len = FB_STRSIZE( dst );

			/* Preserve the old descriptor when growth is unavailable. */
			if( fb_hStrAddSize( dst_len, src_len, &total ) &&
			    fb_hStrRealloc( dstr, total, FB_TRUE ) != NULL )
				fb_hStrCopy( &dstr->data[dst_len], src_ptr, src_len );
		}
		else if( dst_size & FB_STRISFIXED )
		{
			/* do nothing, can't concat to STRING*N
			because it is padded with spaces */
		}
		else
		{
			dst_len = (ssize_t)strlen( (char *)dst );

			/* don't check byte ptr's */
			if( dst_size > 0 )
			{
				/* less the null-term */
				--dst_size;

				if( src_len > dst_size - dst_len )
					src_len = dst_size - dst_len;
			}

			fb_hStrCopy( &(((char *)dst)[dst_len]), src_ptr, src_len );

			/* don't check byte ptr's */
			if( (fillrem != 0) && (dst_size > 0) )
			{
				/* fill reminder with null's */
				dst_size -= (dst_len + src_len);
				if( dst_size > 0 )
					memset( &(((char *)dst)[dst_len+src_len]), 0, (size_t)dst_size );
			}
		}
	}


	/* delete temp? */
	if( src_size == FB_STRSIZEVARLEN )
		fb_hStrDelTemp( (FBSTRING_ACCESS *)src );

	return dst;
}

/* end of strings/str_concatassign.c */
