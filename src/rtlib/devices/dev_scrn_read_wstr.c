/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_read_wstr.c
 * Purpose: Read wide units from the screen character device.
 * Responsibilities: Decode the keyboard queue and handle redirected stdin.
 * This file intentionally does NOT contain: Line editing or INPUT token parsing.
 */
#include "fb.h"

typedef int (*READ_BYTE)( void *source );
typedef void (*UNREAD_BYTE)( void *source, int byte );

/* The screen lock protects the byte queue shared with the narrow reader. */
static int hReadScreenByte( void *source )
{
	DEV_SCRN_INFO *info = (DEV_SCRN_INFO *)source;
	int result;

	if( info->length > sizeof(info->buffer) )
		info->length = 0;
	if( info->length == 0 ) {
		if( fb_ctx.hooks.inkeyproc == NULL ) {
			/* Unlike INKEY, GETKEY also reads a terminal when the
			   native screen editor is unavailable. Skip extended keys. */
			int key;
			do {
				key = fb_Getkey();
			} while( key > 0xFF );
			return key < 0 ? EOF : key;
		}
		while( fb_KeyHit() == 0 )
			fb_Delay( 25 ); /* milliseconds between keyboard polls */
		fb_DevScrnFillInput( info );
		if( info->length == 0 )
			return EOF;
	}

	result = (unsigned char)info->buffer[0];
	--info->length;
	memmove( info->buffer, info->buffer + 1, info->length );
	return result;
}

static void hUnreadScreenByte( void *source, int byte )
{
	DEV_SCRN_INFO *info = (DEV_SCRN_INFO *)source;
	if( info->length < sizeof(info->buffer) ) {
		memmove( info->buffer + 1, info->buffer, info->length );
		info->buffer[0] = (char)(unsigned char)byte;
		++info->length;
	}
}

static int hReadScalar( void *source, READ_BYTE next_byte, UNREAD_BYTE put_back )
{
	int first = next_byte( source );
#if defined(HOST_DOS) || defined(HOST_XBOX) || defined(HOST_WINCE)
	/* Legacy screen drivers deliver code-page bytes rather than UTF-8. */
	(void)put_back;
	return first;
#else
	unsigned char bytes[4];
	ssize_t consumed;
	int width, i, next;
	unsigned int scalar = FB_UTF8_REPLACEMENT;

	if( first < 0x80 )
		return first; /* EOF or ASCII */
	if( first >= 0xC2 && first <= 0xDF )
		width = 2;
	else if( first >= 0xE0 && first <= 0xEF )
		width = 3;
	else if( first >= 0xF0 && first <= 0xF4 )
		width = 4;
	else
		return FB_UTF8_REPLACEMENT;

	bytes[0] = (unsigned char)first;
	for( i = 1; i < width; ++i ) {
		next = next_byte( source );
		if( next == EOF )
			return FB_UTF8_REPLACEMENT;
		bytes[i] = (unsigned char)next;
		consumed = 0;
		scalar = fb_hUtf8Decode( (const char *)bytes, i + 1, &consumed );
		if( consumed != i + 1 ) {
			/* Leave the start of the next character in the stream. */
			put_back( source, next );
			return FB_UTF8_REPLACEMENT;
		}
	}
	return (int)scalar;
#endif
}

static size_t hReadWide( DEV_SCRN_INFO *info, FB_WCHAR *dst, size_t chars,
                         void *source, READ_BYTE next_byte, UNREAD_BYTE put_back )
{
	size_t read = 0;
	while( read < chars ) {
		int scalar;
		if( info->wide_pending != 0 ) {
			dst[read++] = (FB_WCHAR)info->wide_pending;
			info->wide_pending = 0;
			continue;
		}

		scalar = hReadScalar( source, next_byte, put_back );
		if( scalar == EOF )
			break;
		if( sizeof(FB_WCHAR) == 2 && scalar > 0xFFFF ) {
			scalar -= 0x10000;
			dst[read++] = (FB_WCHAR)(0xD800 + (scalar >> 10));
			info->wide_pending = (unsigned)(0xDC00 + (scalar & 0x3FF));
		} else {
			if( sizeof(FB_WCHAR) == 1 && scalar > 0xFF )
				scalar = '?';
			dst[read++] = (FB_WCHAR)scalar;
		}
	}
	if( read < chars ) memset( dst + read, 0, (chars - read) * sizeof(*dst) );
	return read;
}

int fb_DevScrnReadWstr( FB_FILE *handle, FB_WCHAR *dst, size_t *pchars )
{
	size_t chars, read = 0;
	DEV_SCRN_INFO *info;
	FB_FILE *real_handle;

	if( pchars == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	chars = *pchars;
	*pchars = 0;
	if( (chars > (size_t)PTRDIFF_MAX / sizeof(FB_WCHAR)) ||
	    (chars != 0 && dst == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	if( chars == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	FB_LOCK();
	real_handle = FB_HANDLE_DEREF( handle );
	if( real_handle == NULL || real_handle->opaque == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}
	info = (DEV_SCRN_INFO *)real_handle->opaque;

#if defined(HOST_WIN32) && !defined(HOST_WINCE)
	if( info->length == 0 && info->wide_pending == 0 ) {
		int status;
		read = chars;
		if( fb_hConsoleReadWideUnits( dst, &read, &status ) ) {
			FB_UNLOCK();
			if( read < chars ) memset( dst + read, 0, (chars - read) * sizeof(*dst) );
			*pchars = read;
			return fb_ErrorSetNum( status );
		}
		read = 0;
	}
#endif

	read = hReadWide( info, dst, chars, info, hReadScreenByte, hUnreadScreenByte );
	FB_UNLOCK();

	*pchars = read;

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

#if !defined(HOST_DOS) && !defined(HOST_XBOX) && !defined(HOST_WINCE)
static int hReadStreamByte( void *source )
{
	return fgetc( (FILE *)source );
}

static void hUnreadStreamByte( void *source, int byte )
{
	/* stdio guarantees a single byte of pushback after a read. */
	ungetc( byte, (FILE *)source );
}
#endif

static int hReadFromStdin( FB_FILE *handle, FB_WCHAR *dst, size_t *pchars )
{
#if defined(HOST_DOS) || defined(HOST_XBOX) || defined(HOST_WINCE)
	(void)handle;
	return fb_DevFileReadWstr( NULL, dst, pchars );
#else
	size_t requested;
	int status;
	DEV_SCRN_INFO *info;

	if( pchars == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	requested = *pchars;
	*pchars = 0;
	if( requested > (size_t)PTRDIFF_MAX / sizeof(*dst) ||
	    (requested != 0 && dst == NULL) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	if( requested == 0 )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	FB_LOCK();
	info = (DEV_SCRN_INFO *)handle->opaque;
	if( info == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	*pchars = hReadWide( info, dst, requested, stdin, hReadStreamByte, hUnreadStreamByte );
	status = ferror( stdin ) ? FB_RTERROR_FILEIO : FB_RTERROR_OK;
	FB_UNLOCK();
	return fb_ErrorSetNum( status );
#endif
}

void fb_DevScrnInit_ReadWstr( void )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK( );
	if( FB_HANDLE_SCREEN->hooks->pfnReadWstr == NULL )
	{
		FB_HANDLE_SCREEN->hooks->pfnReadWstr =
			(fb_IsRedirected( TRUE )? hReadFromStdin : fb_DevScrnReadWstr);
	}
	FB_UNLOCK( );
}

/* end of devices/dev_scrn_read_wstr.c */
