/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_instrrevany.c
 * Purpose: instrrevanyw function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL ssize_t fb_WstrInstrRevAny( const FB_WCHAR *src, const FB_WCHAR *patt, ssize_t start )
{
	if( (src != NULL) && (patt != NULL) ) 
	{
		ssize_t size_src = fb_wstr_Len(src);
		ssize_t size_patt = fb_wstr_Len(patt);
		ssize_t i;

		if( (size_src != 0) && (size_patt != 0) && (start != 0))
		{
			if( start < 0 )
				start = size_src;
			else if( start > size_src )
				start = 0;

			while( start-- )
			{
				for( i = 0; i != size_patt; ++i )
					if( src[start] == patt[i] )
						return start + 1;
			}
		}
	}

	return 0;
}

/* end of strings/wide/strw_instrrevany.c */
