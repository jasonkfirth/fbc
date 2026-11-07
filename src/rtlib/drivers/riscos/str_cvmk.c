/*
 * Project: FreeBASIC RISC OS CV and MK string conversion
 * ------------------------------------------------------
 *
 * File: drivers/riscos/str_cvmk.c
 *
 * Purpose:
 *
 *     Implement binary numeric string conversion for the APCS layout.
 *
 * Responsibilities:
 *
 *     - decode MKD byte strings into APCS doubles
 *     - encode APCS doubles as canonical MKD byte strings
 *     - preserve the generic integer and single conversion behavior
 *
 * This file intentionally does NOT contain: *
 *     - textual numeric formatting
 *     - file encoding
 *     - non-RISC OS representation policy
 */
/* CV# and MK#$ routines */

#include "../../fb.h"

#include "fb_double_riscos.h"

static void hCV( FBSTRING_ACCESS *str, ssize_t len, void *num )
{
	ssize_t i;

	if( str == NULL )
		return;

	if( (str->data != NULL) && (FB_STRSIZE( str ) >= len) )
	{
		for( i = 0; i < len; i++ )
			((char *)num)[i] = str->data[i];
	}

	/* del if temp */
	fb_hStrDelTemp( str );
}

FBCALL double fb_CVD( FBSTRING_ACCESS *str )
{
	uint64_t bits = 0;
	hCV( str, sizeof( bits ), &bits );
	return fb_riscos_DoubleFromBits( bits );
}

FBCALL float fb_CVS( FBSTRING_ACCESS *str )
{
	float num = 0.0;
	hCV( str, sizeof( float ), &num );
	return num;
}

FBCALL short fb_CVSHORT( FBSTRING_ACCESS *str )
{
	short num = 0;
	hCV( str, sizeof( short ), &num );
	return num;
}

/* 32bit legacy, fbc after 64bit port always calls fb_CVL() or fb_CVLONGINT() */
FBCALL int fb_CVI( FBSTRING_ACCESS *str )
{
	int num = 0;
	hCV( str, sizeof( int ), &num );
	return num;
}

FBCALL int fb_CVL( FBSTRING_ACCESS *str )
{
	int num = 0;
	hCV( str, sizeof( int ), &num );
	return num;
}

FBCALL long long fb_CVLONGINT( FBSTRING_ACCESS *str )
{
	long long num = 0;
	hCV( str, sizeof( long long ), &num );
	return num;
}

static FBSTRING_ACCESS *hMK( ssize_t len, void *num )
{
	ssize_t i;
	FBSTRING_ACCESS *dst;

	/* alloc temp string */
    dst = fb_hStrAllocTemp( NULL, len );
	if( dst != NULL )
	{
		/* convert */
		for( i = 0; i < len; i++ )
			dst->data[i] = ((char *)num)[i];

		dst->data[len] = '\0';
	}
	else
		dst = &fb_ctx.null_desc;

	return dst;
}

FBCALL FBSTRING_ACCESS *fb_MKD( double num )
{
	uint64_t bits = fb_riscos_DoubleToBits( num );
	return hMK( sizeof( bits ), &bits );
}

FBCALL FBSTRING_ACCESS *fb_MKS( float num )
{
	return hMK( sizeof( float ), &num );
}

FBCALL FBSTRING_ACCESS *fb_MKSHORT( short num )
{
	return hMK( sizeof( short ), &num );
}

FBCALL FBSTRING_ACCESS *fb_MKI( ssize_t num )
{
	return hMK( sizeof( num ), &num );
}

FBCALL FBSTRING_ACCESS *fb_MKL( int num )
{
	return hMK( sizeof( num ), &num );
}

FBCALL FBSTRING_ACCESS *fb_MKLONGINT( long long num )
{
	return hMK( sizeof( long long ), &num );
}

/* end of drivers/riscos/str_cvmk.c */
