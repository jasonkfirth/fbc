/* detects EOF for file device */

#include "../fb.h"

int fb_DevFileEof( FB_FILE *handle )
{
    FILE *fp;

	FB_LOCK();

    fp = (FILE*) handle->opaque;

	if( fp == NULL ) {
		FB_UNLOCK();
		return FB_TRUE;
	}

	int eof;
	switch( handle->mode ) {
	/* non-text mode? */
	case FB_FILE_MODE_BINARY:
	case FB_FILE_MODE_RANDOM:
		/* note: handle->putback_size will be checked by fb_FileEofEx() */
		/* This detects both cases: a) last read reached EOF, b) next
		   read will reach EOF */
		eof = (ftello( fp ) >= handle->size);
		break;

	/* text-mode (INPUT, OUTPUT or APPEND) */
	default:
#ifdef HOST_XBOX
		/*
			nxdk's current PDCLib ungetc() path is not reliable enough for
			the one-byte EOF peek below.  Encoded file reads are especially
			sensitive to that because losing a single byte shifts UTF-16 and
			UTF-32 decoding out of alignment.
		*/
		if( handle->size >= 0 ) {
			fb_off_t pos = ftello( fp );
			eof = (pos < 0) ? feof( fp ) : (pos >= handle->size);
			break;
		}
#endif

		/* This also handles the EOF char (27). */
		/* We can't check ftell(), because it's not guaranteed to give
		   a real file offset in text mode. */
		/* a) detect whether last read reached EOF */
		eof = feof( fp );
		if( !eof ) {
			/* b) peek ahead: will the next read reach EOF? */
			int c = getc( fp );
			eof = (c == EOF);
			if( !eof ) {
				ungetc( c, fp );
			}
		}
		break;
	}

	FB_UNLOCK();
	return eof ? FB_TRUE : FB_FALSE;
}
