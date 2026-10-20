/*
 * Project: FreeBASIC runtime library
 * File: strings/str_instrrevany.c
 * Purpose: instrrev function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL ssize_t fb_StrInstrRevAny( FBSTRING *src, FBSTRING *patt, ssize_t start )
{
	ssize_t r = 0;

	if( (src != NULL) && (src->data != NULL) && (patt != NULL) && (patt->data != NULL) ) 
	{
		ssize_t size_src = FB_STRSIZE(src);
		ssize_t size_patt = FB_STRSIZE(patt);

		if( (size_src != 0) && (size_patt != 0) && (start != 0) )
		{
			if( start < 0 )
				start = size_src;
			else if( start > size_src )
				start = 0;

			while( start-- && (r == 0) )
			{
				ssize_t i;
				for( i = 0; i != size_patt; ++i )
				{
					if( src->data[start] == patt->data[i] )
					{	
						r = start + 1;
						break;
					}
				}
			}
		}
	}

	FB_STRLOCK();

	/* del if temp */
	fb_hStrDelTemp_NoLock( src );
	fb_hStrDelTemp_NoLock( patt );

	FB_STRUNLOCK();

	return r;
}

/* end of strings/str_instrrevany.c */
