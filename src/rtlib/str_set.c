/* lset and rset functions */

#include "fb.h"

static void hStrSetEx
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size,
		int is_rset
	)
{
	char *dst_ptr;
	const char *src_ptr;
	ssize_t dlen, slen, len, padlen;
	int dst_is_fixed;

	FB_STRSETUP_DYN( dst, dst_size, dst_ptr, dlen );
	FB_STRSETUP_DYN( src, src_size, src_ptr, slen );

	dst_is_fixed =
		(dst_size != FB_STRSIZEVARLEN) &&
		((dst_size & FB_STRISFIXED) != 0);

	if( (dst_ptr != NULL) && (src_ptr != NULL) && (dlen > 0) )
	{
		if( is_rset )
		{
			padlen = dlen - slen;
			if( padlen > 0 )
				memset( dst_ptr, 32, padlen );
			else
				padlen = 0;
		}
		else
		{
			padlen = 0;
		}

		len = (dlen <= slen? dlen: slen);

		if( dst_is_fixed )
			fb_hStrCopyN( dst_ptr + padlen, src_ptr, len );
		else
			fb_hStrCopy( dst_ptr + padlen, src_ptr, len );

		if( is_rset == FB_FALSE )
		{
			padlen = dlen - slen;
			if( padlen > 0 )
				memset( dst_ptr + slen, 32, padlen );
		}

		if( dst_is_fixed == FB_FALSE )
			dst_ptr[dlen] = '\0';
	}

	if( src_size == FB_STRSIZEVARLEN )
		fb_hStrDelTemp( (FBSTRING *)src );

	if( dst_size == FB_STRSIZEVARLEN )
		fb_hStrDelTemp( (FBSTRING *)dst );
}

FBCALL void fb_StrLsetEx
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size
	)
{
	hStrSetEx( dst, dst_size, src, src_size, FB_FALSE );
}

FBCALL void fb_StrRsetEx
	(
		void *dst,
		ssize_t dst_size,
		void *src,
		ssize_t src_size
	)
{
	hStrSetEx( dst, dst_size, src, src_size, FB_TRUE );
}

FBCALL void fb_StrLset ( FBSTRING *dst, FBSTRING *src )
{
	ssize_t slen, dlen, len;

	if( (dst != NULL) && (dst->data != NULL) && (src != NULL) )
	{
		slen = FB_STRSIZE( src );
		dlen = FB_STRSIZE( dst );

		if( dlen > 0 )
		{
			len = (dlen <= slen? dlen: slen );

			fb_hStrCopy( dst->data, src->data, len );

			len = dlen - slen;
			if( len > 0 )
			{
				memset( &dst->data[slen], 32, len );

				/* null char */
				dst->data[slen+len] = '\0';
			}
		}
	}

	/* del if temp */
	fb_hStrDelTemp( src );

	/* del if temp */
	fb_hStrDelTemp( dst );
}

/* LSET fixed length string from var-len string */
FBCALL void fb_StrLsetANA ( void *dst, ssize_t dst_size, FBSTRING *src )
{
	ssize_t slen, dlen, len;
	char *dst_bytes = (char *)dst;

	if( (dst != NULL) && (src != NULL) )
	{
		slen = FB_STRSIZE( src );
		dlen = dst_size & FB_STRSIZEMSK;

		if( dlen > 0 )
		{
			len = (dlen <= slen? dlen: slen );

			fb_hStrCopyN( dst, src->data, len );

			len = dlen - slen;
			if( len > 0 )
			{
				memset( dst_bytes + slen, 32, len );
			}
		}
	}

	/* del if temp */
	fb_hStrDelTemp( src );
}

FBCALL void fb_StrRset ( FBSTRING *dst, FBSTRING *src )
{
	ssize_t slen, dlen, len, padlen;

	if( (dst != NULL) && (dst->data != NULL) && (src != NULL) )
	{
		slen = FB_STRSIZE( src );
		dlen = FB_STRSIZE( dst );

		if( dlen > 0 )
		{
			padlen = dlen - slen;
			if( padlen > 0 )
				memset( dst->data, 32, padlen );
			else
				padlen = 0;

			len = (dlen <= slen? dlen: slen );

			fb_hStrCopy( &dst->data[padlen], src->data, len );
		}
	}

	/* del if temp */
	fb_hStrDelTemp( src );

	/* del if temp */
	fb_hStrDelTemp( dst );
}

/* RSET fixed length string from var-len string */
FBCALL void fb_StrRsetANA ( void *dst, ssize_t dst_size, FBSTRING *src )
{
	ssize_t slen, dlen, len, padlen;
	char *dst_bytes = (char *)dst;

	if( (dst != NULL) && (src != NULL) )
	{
		slen = FB_STRSIZE( src );
		dlen = dst_size & FB_STRSIZEMSK;

		if( dlen > 0 )
		{
			padlen = dlen - slen;
			if( padlen > 0 )
				memset( dst, 32, padlen );
			else
				padlen = 0;

			len = (dlen <= slen? dlen: slen );

			fb_hStrCopyN( dst_bytes + padlen, src->data, len );
		}
	}

	/* del if temp */
	fb_hStrDelTemp( src );
}
