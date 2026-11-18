/*
 * Project: FreeBASIC runtime library
 * File: console/con_lineinp_wstr.c
 * Purpose: Preserve Unicode in console line records and WSTRING LINE INPUT.
 * Responsibilities: Select the console reader and deliver a complete UTF-8 line.
 * This file intentionally does NOT contain: INPUT token parsing or file encodings.
 */
#include "fb.h"
#include <limits.h>

#if defined(HOST_WIN32) && !defined(HOST_WINCE)
#include <windows.h>

typedef BOOL (WINAPI *FB_READCONSOLEW)( HANDLE, LPVOID, DWORD, LPDWORD, LPVOID );

static FB_READCONSOLEW hGetWideReader( void )
{
	FB_READCONSOLEW reader = NULL;
	HMODULE module = GetModuleHandleA( "kernel32.dll" );
	FARPROC entry = module ? GetProcAddress( module, "ReadConsoleW" ) : NULL;

	/* Keep the ANSI fallback available on systems without this export. */
	if( entry )
		memcpy( &reader, &entry, sizeof(reader) );
	return reader;
}

/* Called under FB_LOCK by the screen device. A false result leaves the
   byte keyboard path in control (graphics hooks and pre-NT consoles). */
int fb_hConsoleReadWideUnits( FB_WCHAR *dst, size_t *chars, int *status )
{
	FB_READCONSOLEW reader;
	HANDLE console;
	DWORD saved, actual = 0;
	DWORD requested = (DWORD)(*chars > 256 ? 256 : *chars);

	if( fb_ctx.hooks.inkeyproc != NULL || (GetVersion() & 0x80000000UL) )
		return FALSE;
	reader = hGetWideReader();
	console = GetStdHandle( STD_INPUT_HANDLE );
	if( reader == NULL || !GetConsoleMode( console, &saved ) )
		return FALSE;
	*status = FB_RTERROR_OK;
	if( !SetConsoleMode( console, saved & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) ) ) {
		*chars = 0;
		*status = FB_RTERROR_FILEIO;
		return TRUE;
	}
	if( !reader( console, dst, requested, &actual, NULL ) )
		*status = FB_RTERROR_FILEIO;
	if( !SetConsoleMode( console, saved ) )
		*status = FB_RTERROR_FILEIO;
	*chars = *status == FB_RTERROR_OK ? (size_t)actual : 0;
	return TRUE;
}

static int hReadWideConsole( FB_READCONSOLEW reader, FBSTRING_ACCESS *dst, int addnewline )
{
	HANDLE console = GetStdHandle( STD_INPUT_HANDLE );
	DWORD saved, mode;
	WCHAR chunk[256];
	WCHAR *units = NULL;
	size_t length = 0, capacity = 0;
	int status = FB_RTERROR_OK;
	int line_done = FALSE;
	int bytes;

	if( !GetConsoleMode( console, &saved ) )
		return FB_RTERROR_FILEIO;
	mode = saved | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
	/* Native echo includes Enter. For LINE INPUT; echo the completed
	   record ourselves so the cursor remains after the input text. */
	if( addnewline )
		mode |= ENABLE_ECHO_INPUT;
	else
		mode &= ~(DWORD)ENABLE_ECHO_INPUT;
	if( !SetConsoleMode( console, mode ) )
		return FB_RTERROR_FILEIO;

	while( !line_done ) {
		DWORD count;
		size_t used, needed, next_capacity;
		WCHAR *grown;

		if( !reader( console, chunk, (DWORD)(sizeof(chunk) / sizeof(chunk[0])), &count, NULL ) ) {
			status = FB_RTERROR_FILEIO;
			break;
		}
		if( count == 0 ) {
			if( length == 0 ) status = FB_RTERROR_ENDOFFILE;
			break;
		}

		for( used = 0; used < (size_t)count; ++used ) {
			if( chunk[used] == L'\n' ) {
				line_done = TRUE;
				break;
			}
		}
		/* WideCharToMultiByte takes an int count. Reserve a bounded
		   buffer for the whole record so chunk boundaries preserve pairs. */
		if( used > (size_t)INT_MAX - length ) {
			status = FB_RTERROR_OUTOFMEM;
			break;
		}
		needed = length + used;
		if( needed > capacity ) {
			next_capacity = capacity > (size_t)INT_MAX / 2 ? (size_t)INT_MAX : capacity * 2;
			if( next_capacity < needed ) next_capacity = needed;
			grown = (WCHAR *)realloc( units, next_capacity * sizeof(*units) );
			if( grown == NULL ) {
				status = FB_RTERROR_OUTOFMEM;
				break;
			}
			units = grown;
			capacity = next_capacity;
		}
		if( used != 0 ) memcpy( units + length, chunk, used * sizeof(*units) );
		length = needed;
	}

	if( !SetConsoleMode( console, saved ) && status == FB_RTERROR_OK )
		status = FB_RTERROR_FILEIO;
	if( length != 0 && units[length - 1] == L'\r' ) --length;

	if( status == FB_RTERROR_OK && length != 0 ) {
		bytes = WideCharToMultiByte( CP_UTF8, 0, units, (int)length, NULL, 0, NULL, NULL );
		if( bytes <= 0 )
			status = FB_RTERROR_FILEIO;
		else if( fb_hStrAlloc( dst, bytes ) == NULL )
			status = FB_RTERROR_OUTOFMEM;
		else if( WideCharToMultiByte( CP_UTF8, 0, units, (int)length, dst->data, bytes,
		                             NULL, NULL ) != bytes ) {
			fb_StrDelete( dst );
			status = FB_RTERROR_FILEIO;
		} else {
			dst->data[bytes] = '\0';
			if( !addnewline )
				fb_PrintBufferWstrEx( units, length, 0 );
		}
	}
	free( units );
	return status;
}

static void hPrintPrompt( FBSTRING_ACCESS *text, int addquestion )
{
	if( text != NULL ) {
		if( text->data != NULL )
			fb_PrintString( 0, text, 0 );
		else
			fb_hStrDelTemp( text );
		if( addquestion )
			fb_PrintFixString( 0, "? ", 0 );
	}
}
#endif

int fb_hConsoleLineInputUtf8( FBSTRING_ACCESS *text, FBSTRING_ACCESS *dst,
                              int addquestion, int addnewline )
{
#if defined(HOST_WIN32) && !defined(HOST_WINCE)
	FB_READCONSOLEW reader;

	FB_LOCK();
	reader = fb_ctx.hooks.lineinputproc == NULL ? hGetWideReader() : NULL;
	if( reader != NULL && !fb_IsRedirected( TRUE ) &&
	    !(GetVersion() & 0x80000000UL) ) {
		int status;
		fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );
		hPrintPrompt( text, addquestion );
		status = hReadWideConsole( reader, dst, addnewline );
		FB_UNLOCK();
		return fb_ErrorSetNum( status );
	}
	/* With redirected stdin there is no Windows console input buffer. */
	if( fb_ctx.hooks.lineinputproc == NULL && fb_IsRedirected( TRUE ) ) {
		int status;
		hPrintPrompt( text, addquestion );
		status = fb_DevFileReadLineDumb( stdin, dst, NULL );
		FB_UNLOCK();
		return fb_ErrorSetNum( status );
	}
	FB_UNLOCK();
#endif
	/* Terminal and graphics hooks deliver a byte record. Leave existing
	   editing and cursor handling to the corresponding narrow hook. */
	return fb_LineInput( text, dst, -1, FALSE, addquestion, addnewline );
}

int fb_ConsoleLineInputWstr( const FB_WCHAR *text, FB_WCHAR *dst, ssize_t max_chars,
                             int addquestion, int addnewline )
{
	FBSTRING line = { 0, 0, 0 };
	int status;

	fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );
	if( text != NULL ) {
		fb_PrintWstr( 0, text, 0 );
		if( addquestion )
			fb_PrintFixString( 0, "? ", 0 );
	}

	status = fb_hConsoleLineInputUtf8( NULL, &line, FALSE, addnewline );
	fb_hConsoleBytesToWstr( dst, max_chars, line.data, FB_STRSIZE(&line) );
	fb_StrDelete( &line );
	return fb_ErrorSetNum( status );
}

/* end of console/con_lineinp_wstr.c */
