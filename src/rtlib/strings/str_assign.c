/*
 * Project: FreeBASIC runtime library
 * File: strings/str_assign.c
 * Purpose: string assigning function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include "str_size.h"

FBCALL void *fb_StrAssignEx
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size,
		int fill_rem,
		int is_init
	)
{
	FBSTRING_ACCESS *dstr;
	const char *src_ptr;
	ssize_t src_len;

	FB_STRLOCK();

	if( dst == NULL )
	{
		if( src_size == FB_STRSIZEVARLEN )
			fb_hStrDelTemp_NoLock( (FBSTRING_ACCESS *)src );

		FB_STRUNLOCK();

		return dst;
	}

	/* src */
	FB_STRSETUP_FIX( src, src_size, src_ptr, src_len );

	/* is dst var-len? */
	if( dst_size == FB_STRSIZEVARLEN )
	{
        dstr = (FBSTRING_ACCESS *)dst;

		/* src NULL? */
		if( src_len == 0 )
		{
			if( is_init == FB_FALSE )
			{
				fb_StrDelete( dstr );
			}
			else
			{
				dstr->data = NULL;
				dstr->len = 0;
				dstr->size = 0;
			}
		}
		else
		{
			/* if src is a temp, just copy the descriptor */
			if( (src_size == FB_STRSIZEVARLEN) && FB_ISTEMP(src) )
			{
				if( is_init == FB_FALSE )
					fb_StrDelete( dstr );

				dstr->data = ((FBSTRING_ACCESS *)src)->data;
				dstr->len = src_len;
				dstr->size = ((FBSTRING_ACCESS *)src)->size;

				((FBSTRING_ACCESS *)src)->data = NULL;
				((FBSTRING_ACCESS *)src)->len = 0;
				((FBSTRING_ACCESS *)src)->size = 0;

				fb_hStrDelTempDesc( (FBSTRING_ACCESS *)src );

				FB_STRUNLOCK();

				return dst;
			}

        	/* else, realloc dst if needed and copy src */
        	if( is_init == FB_FALSE )
        	{
				if( FB_STRSIZE( dst ) != src_len &&
				    fb_hStrRealloc( dstr, src_len, FB_FALSE ) == NULL )
					goto cleanup;
        	}
        	else
        	{
				if( fb_hStrAlloc( dstr, src_len ) == NULL )
					goto cleanup;
        	}

			fb_hStrCopy( dstr->data, src_ptr, src_len );
		}
	}
	/* fixed-len string */
	else if( dst_size & FB_STRISFIXED )
	{
		dst_size &= FB_STRSIZEMSK;

		if( src_len > 0 )
		{
			if( dst_size < src_len )
				src_len = dst_size;

			fb_hStrCopyN( (char *)dst, src_ptr, src_len );
		}

		dst_size -= src_len;
		if( dst_size > 0 )
		{
			memset( &(((char *)dst)[src_len]), 32, (size_t)dst_size );
		}
	}
	/* fixed-len zstring or zstring ptr */
	else
	{
		/* src NULL? */
		if( src_len == 0 )
		{
			*(char *)dst = '\0';
		}
		else
		{
			/* byte ptr? as in C, assume dst is large enough */
			if( dst_size == 0 )
				dst_size = src_len;
			else
			{
				/* less the null-term */
				--dst_size;

				if( dst_size < src_len )
					src_len = dst_size;
			}

			fb_hStrCopy( (char *)dst, src_ptr, src_len );
		}

		/* fill reminder with null's */
		if( fill_rem != 0 )
		{
			dst_size -= src_len;
			if( dst_size > 0 )
				memset( &(((char *)dst)[src_len]), 0, (size_t)dst_size );
		}
	}


cleanup:
	/* Allocation failure must still retire a source temporary. */
	/* delete temp? */
	if( src_size == FB_STRSIZEVARLEN )
		fb_hStrDelTemp_NoLock( (FBSTRING_ACCESS *)src );

	FB_STRUNLOCK();

	return dst;
}

FBCALL void *fb_StrAssign
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size,
		int fill_rem
	)
{
	return fb_StrAssignEx( dst, dst_size, src, src_size, fill_rem, FB_FALSE );
}

FBCALL void *fb_StrInit
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size,
		int fill_rem
	)
{
	return fb_StrAssignEx( dst, dst_size, src, src_size, fill_rem, FB_TRUE );
}

/* end of strings/str_assign.c */
