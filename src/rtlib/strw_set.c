/* lsetw and rsetw functions */

#include "fb.h"

static ssize_t hWstrSetLength
	(
		const FB_WCHAR *str,
		ssize_t encoded_size
	)
{
	if( str == NULL )
		return 0;

	if( encoded_size <= 0 )
		return fb_wstr_Len( str );

	return encoded_size - 1;
}

static void hWstrSetEx
	(
		FB_WCHAR *dst,
		ssize_t dst_size,
		const FB_WCHAR *src,
		ssize_t src_size,
		int is_rset
	)
{
	ssize_t slen, dlen, len, padlen;

	if( (dst == NULL) || (src == NULL) )
		return;

	slen = hWstrSetLength( src, src_size );
	dlen = hWstrSetLength( dst, dst_size );

	if( dlen <= 0 )
		return;

	if( is_rset )
	{
		padlen = dlen - slen;
		if( padlen > 0 )
			fb_wstr_Fill( dst, 32, padlen );
		else
			padlen = 0;
	}
	else
	{
		padlen = 0;
	}

	len = (dlen <= slen? dlen: slen);
	fb_wstr_Copy( &dst[padlen], src, len );

	if( is_rset == FB_FALSE )
	{
		padlen = dlen - slen;
		if( padlen > 0 )
			fb_wstr_Fill( &dst[slen], 32, padlen );
	}

	dst[dlen] = _LC('\0');
}

FBCALL void fb_WstrLsetEx
	(
		FB_WCHAR *dst,
		ssize_t dst_size,
		const FB_WCHAR *src,
		ssize_t src_size
	)
{
	hWstrSetEx( dst, dst_size, src, src_size, FB_FALSE );
}

FBCALL void fb_WstrRsetEx
	(
		FB_WCHAR *dst,
		ssize_t dst_size,
		const FB_WCHAR *src,
		ssize_t src_size
	)
{
	hWstrSetEx( dst, dst_size, src, src_size, FB_TRUE );
}

FBCALL void fb_WstrLset ( FB_WCHAR *dst, FB_WCHAR *src )
{
	ssize_t slen, dlen, len;

	if( (dst != NULL) && (src != NULL) )
	{
		slen = fb_wstr_Len( src );
		dlen = fb_wstr_Len( dst );

		if( dlen > 0 )
		{
			len = (dlen <= slen? dlen: slen );

			fb_wstr_Copy( dst, src, len );

			len = dlen - slen;
			if( len > 0 )
				fb_wstr_Fill( &dst[slen], 32, len );
		}
	}
}

FBCALL void fb_WstrRset ( FB_WCHAR *dst, FB_WCHAR *src )
{
	ssize_t slen, dlen, len, padlen;

	if( (dst != NULL) && (src != NULL) )
	{
		slen = fb_wstr_Len( src );
		dlen = fb_wstr_Len( dst );

		if( dlen > 0 )
		{
			padlen = dlen - slen;
			if( padlen > 0 )
				fb_wstr_Fill( dst, 32, padlen );
			else
				padlen = 0;

			len = (dlen <= slen? dlen: slen );

			fb_wstr_Copy( &dst[padlen], src, len );
		}
	}
}
