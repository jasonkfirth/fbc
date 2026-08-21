/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: qb_str_convto_flt.c

    Purpose:

        Convert floating-point values to QB-compatible STR() results without
        relying on the broken Windows CE %g formatter.

    Responsibilities:

        * allocate temporary result strings with sufficient capacity
        * request QB leading-space behavior from fb_hFloat2Str()
        * publish the actual formatted length in each string descriptor

    This file intentionally does NOT contain:

        * the significant-digit formatting algorithm
        * non-QB STR() behavior
        * compiler constant folding
*/

#include "../fb.h"


/*:::::*/
FBCALL FBSTRING *fb_FloatToStrQB ( float num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, 7+8 );
	if( dst != NULL )
	{
		size_t tmp_len;

		fb_hFloat2Str( (double)num, dst->data, 7, FB_F2A_ADDBLANK );

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
FBCALL FBSTRING *fb_DoubleToStrQB ( double num )
{
	FBSTRING 	*dst;

	/* alloc temp string */
	dst = fb_hStrAllocTemp( NULL, 16+8 );
	if( dst != NULL )
	{
		size_t tmp_len;

		fb_hFloat2Str( num, dst->data, 16, FB_F2A_ADDBLANK );

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

/* end of qb_str_convto_flt.c */
