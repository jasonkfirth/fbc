/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: str_convto_flt.c

    Purpose:

        Convert single- and double-precision values to FreeBASIC STR()
        results without relying on the broken Windows CE %g formatter.

    Responsibilities:

        * allocate temporary result strings with sufficient capacity
        * delegate significant-digit formatting to fb_hFloat2Str()
        * publish the actual formatted length in each string descriptor

    This file intentionally does NOT contain:

        * the significant-digit formatting algorithm
        * QB-compatible leading-space behavior
        * compiler constant folding
*/

#include "../fb.h"


/*:::::*/
FBCALL FBSTRING *fb_FloatToStr ( float num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, 7+8 );
	if( dst != NULL )
	{
		size_t tmp_len;

		fb_hFloat2Str( (double)num, dst->data, 7, 0 );

		tmp_len = strlen( dst->data );				/* fake len */

		/* skip the dot at end if any */
		if( tmp_len > 0 )
		{
			if( dst->data[tmp_len-1] == '.' )
			{
				dst->data[tmp_len-1] = '\0';
				--tmp_len;
			}
		}
		fb_hStrSetLength( dst, tmp_len );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/*:::::*/
FBCALL FBSTRING *fb_DoubleToStr ( double num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, 16+8 );
	if( dst != NULL )
	{
		size_t tmp_len;

		fb_hFloat2Str( num, dst->data, 16, 0 );

		tmp_len = strlen( dst->data );				/* fake len */

		/* skip the dot at end if any */
		if( tmp_len > 0 )
		{
			if( dst->data[tmp_len-1] == '.' )
			{
				dst->data[tmp_len-1] = '\0';
				--tmp_len;
			}
		}
		fb_hStrSetLength( dst, tmp_len );
	}
	else
		dst = &__fb_ctx.null_desc;

	return dst;
}

/* end of str_convto_flt.c */
