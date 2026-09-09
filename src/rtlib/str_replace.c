/*
    FreeBASIC Runtime Library
    File: str_replace.c
    Purpose: Implement the optional VB-style Replace function on byte strings.
    Responsibilities: Bounded replacement, exact allocation, temporary cleanup.
    This file does not parse patterns or implement Unicode/locale collation.
*/

#include "fb.h"
#include "str_vb_private.h"

FBCALL FBSTRING *fb_StrReplace( FBSTRING *src, FBSTRING *find,
                               FBSTRING *replacement, ssize_t start,
                               ssize_t count, int compare )
{
	FBSTRING *dst = &__fb_ctx.null_desc;
	ssize_t src_len, find_len, repl_len, offset, pos, matches, length, written;
	int error = FB_RTERROR_OK;

	/* Read inputs before deleting any temporary: all three may alias.
	   The string lock protects descriptor allocation, not caller-owned data. */
	FB_STRLOCK();
	src_len = (src && src->data) ? FB_STRSIZE( src ) : 0;
	find_len = (find && find->data) ? FB_STRSIZE( find ) : 0;
	repl_len = (replacement && replacement->data) ? FB_STRSIZE( replacement ) : 0;
	if( start < 1 || count < -1 || (compare != 0 && compare != 1) ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}
	if( start > src_len )
		goto done;

	/* VB returns only the suffix beginning at start, even when no match is
	   replaced. Never prepend the skipped part of the input. */
	offset = start - 1;
	length = src_len - offset;
	if( length > FB_STR_VB_MAXLEN ) {
		error = FB_RTERROR_OUTOFMEM;
		goto done;
	}
	matches = 0;
	if( find_len > 0 && count != 0 ) {
		pos = offset;
		while( find_len <= src_len - pos && (count < 0 || matches < count) ) {
			if( fb_hStrCompareBytes( src->data + pos, find->data, find_len, compare ) == 0 ) {
				if( repl_len > find_len && repl_len - find_len > FB_STR_VB_MAXLEN - length ) {
					error = FB_RTERROR_OUTOFMEM;
					goto done;
				}
				length += repl_len - find_len;
				++matches;
				pos += find_len;
			} else {
				++pos;
			}
		}
	}
	if( length == 0 )
		goto done;
	dst = fb_hStrAllocTemp_NoLock( NULL, length );
	if( dst == NULL ) {
		dst = &__fb_ctx.null_desc;
		error = FB_RTERROR_OUTOFMEM;
		goto done;
	}

	/* A second pass avoids repeated reallocations and never scans inserted
	   text. Matches are non-overlapping in the original input. */
	pos = offset;
	written = 0;
	while( matches > 0 ) {
		if( fb_hStrCompareBytes( src->data + pos, find->data, find_len, compare ) == 0 ) {
			if( repl_len > 0 )
				memcpy( dst->data + written, replacement->data, repl_len );
			written += repl_len;
			pos += find_len;
			--matches;
		} else {
			dst->data[written++] = src->data[pos++];
		}
	}
	if( pos < src_len )
		memcpy( dst->data + written, src->data + pos, src_len - pos );
	dst->data[length] = '\0';

done:
	fb_hStrDelTemp_NoLock( src );
	if( find != src )
		fb_hStrDelTemp_NoLock( find );
	if( replacement != src && replacement != find )
		fb_hStrDelTemp_NoLock( replacement );
	FB_STRUNLOCK();
	fb_ErrorSetNum( error );
	return dst;
}

/* end of str_replace.c */
