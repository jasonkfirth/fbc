/* str$ routines for float and double
 *
 * the result string's len is being "faked" to appear as if it were shorter
 * than the one that has to be allocated to fit _itoa and _gvct buffers.
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

#ifdef HOST_XBOX
		fb_hFloat2Str( (double)num, dst->data, 7, 0 );
#else
		/* convert */
		sprintf( dst->data, "%.7g", num );
#endif

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

#ifdef HOST_XBOX
		fb_hFloat2Str( num, dst->data, 16, 0 );
#else
		/* convert */
		sprintf( dst->data, "%.16g", num );
#endif

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
