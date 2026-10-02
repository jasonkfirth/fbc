/*
    FreeBASIC Runtime Library
    File: ustr_io.c
    Purpose: Keep USTRING text intact across DATA and encoded text files.
    Responsibilities: Read scalars, parse INPUT tokens, and transcode output.
    This file intentionally does NOT contain compiler grammar or console setup.

    Encoded disk files are stdio streams opened by dev_file_encod_open.c;
    their BOM has already been consumed. Reading their ANSI hook would lose
    Unicode information, so this module reads the original encoding directly.
    The file lock protects stream positions, putback state, and hook changes.
*/

#include "fb.h"

typedef struct {
	FB_FILE *file;
	FB_INPUTCTX *input;
	int pending;
	int index_before;
	char raw[4];
	size_t raw_size;
	int status;
} USTR_READER;

/* ------------------------------------------------------------------------- */
/* Scalar input and the existing file putback ABI                             */
/* ------------------------------------------------------------------------- */

static int hEncoded( FB_FILE *file )
{
	/* FileOpenVfsEx marks ordinary disk files as VFS handles too. The
	   encoded stdio hook, rather than the type field, identifies opaque's
	   FILE layout. This also avoids treating a custom VFS pointer as FILE. */
	return FB_HANDLE_USED(file) && (file->encod != FB_FILE_ENCOD_ASCII) &&
	       (file->hooks->pfnRead == fb_DevFileReadEncod) && (file->opaque != NULL);
}

static int hByte( USTR_READER *reader )
{
	unsigned char byte;
	size_t count;
	int value;
	if( hEncoded( reader->file ) ) {
		value = fgetc( (FILE *)reader->file->opaque );
		if( value == EOF && ferror((FILE *)reader->file->opaque) ) reader->status = FB_RTERROR_FILEIO;
		return value;
	}
	if( FB_HANDLE_USED(reader->file) ) {
		reader->status = fb_FileGetDataEx( reader->file, 0, &byte, 1, &count, FALSE, FALSE );
		if( reader->status || count == 0 ) return EOF;
		return byte;
	}
	if( (reader->input == NULL) || (reader->input->index >= FB_STRSIZE(&reader->input->str)) ) return EOF;
	return (unsigned char)reader->input->str.data[reader->input->index++];
}

static void hUnreadByte( USTR_READER *reader, int byte )
{
	unsigned char value = byte;
	if( byte == EOF ) return;
	if( hEncoded( reader->file ) ) ungetc( byte, (FILE *)reader->file->opaque );
	else if( FB_HANDLE_USED(reader->file) ) fb_FilePutBackEx( reader->file, &value, 1 );
	else if( reader->input != NULL && reader->input->index > 0 ) --reader->input->index;
}

static int hUnit( USTR_READER *reader, int bytes )
{
	/* The encoded file device uses little-endian units after the BOM:
	   UTF-16 has two-byte units, UTF-32 has four-byte scalar values. */
	unsigned char buffer[4];
	size_t count = fread( buffer, 1, bytes, (FILE *)reader->file->opaque );
	if( ferror((FILE *)reader->file->opaque) ) reader->status = FB_RTERROR_FILEIO;
	if( count == 0 ) return EOF;
	if( count != (size_t)bytes ) return FB_UTF8_REPLACEMENT;
	if( bytes == 4 && fb_UTF32FromLE( buffer ) > 0x10FFFF ) return FB_UTF8_REPLACEMENT;
	return bytes == 2 ? fb_UTF16FromLE( buffer ) : (int)fb_UTF32FromLE( buffer );
}

static int hReadScalar( USTR_READER *reader )
{
	char bytes[4];
	int scalar, low, byte, required, i;
	ssize_t offset = 0;
	FB_FILE *file = reader->file;
	reader->raw_size = 0;
	if( reader->input != NULL && !FB_HANDLE_USED(file) ) reader->index_before = reader->input->index;
	if( reader->pending != EOF ) {
		scalar = reader->pending;
		reader->pending = EOF;
		return scalar;
	}
	if( hEncoded(file) && file->putback_size > 0 ) {
		if( file->putback_size > sizeof(file->putback_buffer) ||
		    (sizeof(FB_WCHAR) > 1 && file->putback_size % sizeof(FB_WCHAR) != 0) ) {
			file->putback_size = 0;
			reader->status = FB_RTERROR_FILEIO;
			return EOF;
		}
		if( sizeof(FB_WCHAR) == 1 ) {
			scalar = fb_hUtf8Decode( file->putback_buffer, file->putback_size, &offset );
		} else {
			FB_WCHAR unit;
			memcpy( &unit, file->putback_buffer, sizeof(unit) );
			scalar = unit;
			offset = sizeof(unit);
			if( sizeof(FB_WCHAR) == 2 && scalar >= 0xD800 && scalar <= 0xDBFF && file->putback_size >= 4 ) {
				memcpy( &unit, file->putback_buffer + 2, sizeof(unit) );
				if( unit >= 0xDC00 && unit <= 0xDFFF ) {
					scalar = 0x10000 + ((scalar - 0xD800) << 10) + unit - 0xDC00;
					offset = 4;
				}
			}
		}
		file->putback_size -= offset;
		memmove( file->putback_buffer, file->putback_buffer + offset, file->putback_size );
		return scalar;
	}
	if( hEncoded(file) && (file->encod == FB_FILE_ENCOD_UTF16 || file->encod == FB_FILE_ENCOD_UTF32) ) {
		scalar = hUnit( reader, file->encod == FB_FILE_ENCOD_UTF16 ? 2 : 4 );
		if( scalar == EOF ) return EOF;
		if( file->encod == FB_FILE_ENCOD_UTF16 && scalar >= 0xD800 && scalar <= 0xDBFF ) {
			low = hUnit( reader, 2 );
			if( low >= 0xDC00 && low <= 0xDFFF ) return 0x10000 + ((scalar - 0xD800) << 10) + low - 0xDC00;
			reader->pending = low;
			return FB_UTF8_REPLACEMENT;
		}
		if( scalar < 0 || scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF) ) return FB_UTF8_REPLACEMENT;
		return scalar;
	}
	byte = hByte( reader );
	if( byte == EOF ) return EOF;
	bytes[0] = byte;
	reader->raw[0] = byte;
	reader->raw_size = 1;
	if( byte < 0x80 ) return byte;
	if( byte >= 0xC2 && byte <= 0xDF ) required = 2;
	else if( byte >= 0xE0 && byte <= 0xEF ) required = 3;
	else if( byte >= 0xF0 && byte <= 0xF4 ) required = 4;
	else return FB_UTF8_REPLACEMENT;
	for( i = 1; i < required; ++i ) {
		byte = hByte( reader );
		if( byte == EOF ) return FB_UTF8_REPLACEMENT;
		bytes[i] = byte;
		offset = 0;
		fb_hUtf8Decode( bytes, i + 1, &offset );
		if( offset != i + 1 ) {
			hUnreadByte( reader, byte );
			return FB_UTF8_REPLACEMENT;
		}
		reader->raw[i] = byte;
		reader->raw_size = i + 1;
	}
	offset = 0;
	return fb_hUtf8Decode( bytes, required, &offset );
}

static void hPutback( USTR_READER *reader )
{
	char bytes[4];
	ssize_t count;
	FB_WCHAR wide[2];
	unsigned int scalar;
	if( reader->pending == EOF ) return;
	scalar = reader->pending;
	if( hEncoded(reader->file) && sizeof(FB_WCHAR) > 1 ) {
		count = 1;
		wide[0] = scalar;
		if( sizeof(FB_WCHAR) == 2 && scalar > 0xFFFF ) {
			scalar -= 0x10000;
			wide[0] = 0xD800 + (scalar >> 10);
			wide[1] = 0xDC00 + (scalar & 0x3FF);
			count = 2;
		}
		fb_FilePutBackWstrEx( reader->file, wide, count );
	} else {
		count = fb_hUtf8Encode( bytes, scalar );
		if( FB_HANDLE_USED(reader->file) ) {
			/* Keep the original byte prefix on unencoded streams. Its size
			   can differ from U+FFFD, and also matters for file positions. */
			if( !hEncoded(reader->file) && reader->raw_size > 0 )
				fb_FilePutBackEx( reader->file, reader->raw, reader->raw_size );
			else fb_FilePutBackEx( reader->file, bytes, count );
		}
		else if( reader->input != NULL ) reader->input->index = reader->index_before;
	}
}

static int hAppend( FBSTRING *text, unsigned int scalar )
{
	char bytes[4];
	ssize_t count = fb_hUtf8Encode( bytes, scalar );
	if( text->len > FB_USTRING_MAX_BYTES - count ||
	    fb_hStrRealloc( text, text->len + count, TRUE ) == NULL ) return FB_RTERROR_OUTOFMEM;
	memcpy( text->data + text->len - count, bytes, count );
	text->data[text->len] = 0;
	return FB_RTERROR_OK;
}

static void hCommit( FBSTRING *dst, FBSTRING *text )
{
	FB_STRLOCK();
	fb_StrDelete( dst );
	*dst = *text;
	text->data = NULL;
	FB_STRUNLOCK();
}

/* ------------------------------------------------------------------------- */
/* LINE INPUT, INPUT, and DATA                                                */
/* ------------------------------------------------------------------------- */

FBCALL int fb_UStrFileLineInput( int fnum, void *destination, ssize_t size, int fill )
{
	FBSTRING *dst = destination;
	USTR_READER reader = { .file = FB_FILE_TO_HANDLE(fnum), .pending = EOF };
	FBSTRING text = { NULL, 0, 0 };
	int scalar, status = FB_RTERROR_OK;
	FB_LOCK();
	if( !FB_HANDLE_USED(reader.file) || dst == NULL ) status = FB_RTERROR_ILLEGALFUNCTIONCALL;
	else if( !hEncoded(reader.file) ) {
		status = fb_FileLineInput( fnum, dst, -1, 0 );
		fb_UStrAssign( dst, -1, dst, -1, 0 );
	} else {
		while( (scalar = hReadScalar( &reader )) != EOF ) {
			if( scalar == '\n' ) break;
			if( scalar == '\r' ) {
				scalar = hReadScalar( &reader );
				if( scalar != '\n' ) reader.pending = scalar;
				break;
			}
			status = hAppend( &text, scalar );
			if( status ) break;
		}
		hPutback( &reader );
		if( !status ) status = reader.status;
		if( !status ) hCommit( dst, &text );
	}
	fb_StrDelete( &text );
	FB_UNLOCK();
	return fb_ErrorSetNum( status );
}

FBCALL int fb_UStrInput( void *destination, ssize_t size, int fill )
{
	FB_INPUTCTX *input = FB_TLSGETCTX( INPUT );
	FBSTRING *dst = destination;
	USTR_READER reader;
	if( dst == NULL ) return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	if( input == NULL ) return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	reader = (USTR_READER){ .file = input->handle, .input = input, .pending = EOF };
	FBSTRING text = { NULL, 0, 0 };
	int scalar, quoted = FALSE, delimiter = TRUE, status = FB_RTERROR_OK;
	FB_LOCK();
	do scalar = hReadScalar( &reader ); while( scalar == ' ' || scalar == '\t' );
	while( scalar != EOF ) {
		if( scalar == '\r' || scalar == '\n' ) { delimiter = FALSE; break; }
		if( scalar == '"' && text.len == 0 && !quoted ) quoted = TRUE;
		else if( scalar == '"' && quoted ) { scalar = hReadScalar( &reader ); break; }
		else if( scalar == ',' && !quoted ) { delimiter = FALSE; break; }
		else if( (scalar == ' ' || scalar == '\t') && !quoted ) break;
		else if( (status = hAppend( &text, scalar )) != 0 ) break;
		scalar = hReadScalar( &reader );
	}
	if( scalar == '\r' || delimiter ) {
		while( scalar == ' ' || scalar == '\t' ) scalar = hReadScalar( &reader );
		if( scalar == '\r' ) {
			scalar = hReadScalar( &reader );
			if( scalar != '\n' ) reader.pending = scalar;
		} else if( scalar != ',' && scalar != '\n' && scalar != EOF ) reader.pending = scalar;
	}
	hPutback( &reader );
	if( !status ) status = reader.status;
	if( !status ) hCommit( dst, &text );
	fb_StrDelete( &text );
	FB_UNLOCK();
	return fb_ErrorSetNum( status );
}

FBCALL void fb_UStrDataRead( void *destination, ssize_t size, int fill )
{
	FBSTRING *dst = destination;
	/* DATA's packed descriptor carries a 16-bit length and a pointer.
	   The high length bit identifies wide text; other text lengths are
	   byte counts. data.c owns the shared cursor, so reading and advancing
	   it must be one operation under the runtime lock. */
	FB_LOCK();
	if( __fb_data_ptr == NULL || __fb_data_ptr->len == FB_DATATYPE_OFS ) fb_UStrAssign( dst, -1, NULL, 0, 0 );
	else if( __fb_data_ptr->len & FB_DATATYPE_WSTR )
		fb_UStrAssign( dst, -1, fb_UStrFromWstr( __fb_data_ptr->wstr ), -1, 0 );
	else
		fb_UStrAssign( dst, -1, __fb_data_ptr->zstr, FB_STRISFIXED | __fb_data_ptr->len, 0 );
	fb_DataNext();
	FB_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* Encoded file output                                                       */
/* ------------------------------------------------------------------------- */

static int hWriteEncoded( FB_FILE *file, const void *data, size_t length )
{
	const char *text = data;
	unsigned char buffer[256];
	ssize_t offset = 0;
	size_t used = 0;
	unsigned int scalar;
	FILE *stream = file->opaque;
	if( file->encod == FB_FILE_ENCOD_UTF8 )
		return fwrite( data, 1, length, stream ) == length ? 0 : FB_RTERROR_FILEIO;
	if( length > (size_t)FB_USTRING_MAX_BYTES ) return FB_RTERROR_OUTOFMEM;
	while( offset < (ssize_t)length ) {
		scalar = fb_hUtf8Decode( text, length, &offset );
		if( file->encod == FB_FILE_ENCOD_UTF32 ) {
			fb_UTF32ToLE( buffer + used, scalar );
			used += 4;
		} else if( scalar > 0xFFFF ) {
			scalar -= 0x10000;
			fb_UTF16ToLE( buffer + used, 0xD800 + (scalar >> 10) );
			fb_UTF16ToLE( buffer + used + 2, 0xDC00 + (scalar & 0x3FF) );
			used += 4;
		} else {
			fb_UTF16ToLE( buffer + used, scalar );
			used += 2;
		}
		if( used > sizeof(buffer) - 4 ) {
			if( fwrite( buffer, 1, used, stream ) != used ) return FB_RTERROR_FILEIO;
			used = 0;
		}
	}
	/* Empty input or an exact buffer flush leaves no bytes to publish. */
	if( used == 0 ) return FB_RTERROR_OK;
	return fwrite( buffer, 1, used, stream ) == used ? 0 : FB_RTERROR_FILEIO;
}

static void hPrint( int fnum, FBSTRING *src, int mask, int write )
{
	FB_FILE_HOOKS hooks, *original = NULL;
	FB_FILE *file;
	FB_LOCK();
	file = FB_HANDLE_DEREF( FB_FILE_TO_HANDLE(fnum) );
	if( hEncoded(file) ) {
		/* Scope this UTF-8 write hook to one statement. The original stdio
		   stream and encoding stay intact, and the global file lock prevents
		   another thread from observing the temporary hook table. */
		original = file->hooks;
		hooks = *original;
		hooks.pfnWrite = hWriteEncoded;
		file->hooks = &hooks;
	}
	if( write ) fb_WriteString( fnum, src, mask );
	else fb_PrintString( fnum, src, mask );
	if( original != NULL ) file->hooks = original;
	FB_UNLOCK();
}

FBCALL void fb_UStrPrint( int fnum, FBSTRING *src, int mask )
{
	hPrint( fnum, src, mask, FALSE );
}

FBCALL void fb_UStrWrite( int fnum, FBSTRING *src, int mask )
{
	hPrint( fnum, src, mask, TRUE );
}

/* end of ustr_io.c */
