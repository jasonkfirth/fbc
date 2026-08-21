/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/fb_wince.h

    Purpose:

        Define the C runtime and Windows API conventions used by the
        Windows CE runtime implementation.

    Responsibilities:

        - define WinCE-compatible file offset behavior
        - define text, calling convention, and console constants
        - declare executable-path, environment, and locking entry points

    This file intentionally does NOT contain:

        - desktop Win32 compatibility aliases
        - console, file, or thread implementations
        - graphics or sound backend declarations
*/

#ifndef __FB_WINCE_H__
#define __FB_WINCE_H__

#include <malloc.h>
#include <sys/types.h>

/* CeGCC exposes the normal C spellings directly. */

#ifdef HOST_X86
#define FBCALL __stdcall
#else
#define FBCALL
#endif

/* newline for console/file I/O */
#define FB_NEWLINE "\r\n"
#define FB_NEWLINE_WSTR _LC("\r\n")

/* newline for printer I/O */
#define FB_BINARY_NEWLINE "\r\n"
#define FB_BINARY_NEWLINE_WSTR _LC("\r\n")

#define FB_LL_FMTMOD "ll"

/* CeGCC declares the Microsoft spelling but not MinGW's old-name alias. */
#define get_osfhandle _get_osfhandle

#define FB_CONSOLE_MAXPAGES 4

/*
    The CeGCC stdio layer exposes only the ISO C fseek()/ftell() pair.  Its
    file positions are therefore 32-bit even though the FreeBASIC API accepts
    wider offsets.  WinCE file operations provide the checked narrowing at
    the point where a FreeBASIC position reaches this stdio interface.
*/
typedef long fb_off_t;
#define fseeko(stream, offset, whence) fseek(stream, offset, whence)
#define ftello(stream)                 ftell(stream)

#define FB_COLOR_BLACK    (0)
#define FB_COLOR_BLUE     (FOREGROUND_BLUE)
#define FB_COLOR_GREEN    (FOREGROUND_GREEN)
#define FB_COLOR_CYAN     (FOREGROUND_GREEN|FOREGROUND_BLUE)
#define FB_COLOR_RED      (FOREGROUND_RED)
#define FB_COLOR_MAGENTA  (FOREGROUND_RED|FOREGROUND_BLUE)
#define FB_COLOR_BROWN    (FOREGROUND_RED|FOREGROUND_GREEN)
#define FB_COLOR_WHITE    (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define FB_COLOR_GREY     (FOREGROUND_INTENSITY)
#define FB_COLOR_LBLUE    (FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define FB_COLOR_LGREEN   (FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define FB_COLOR_LCYAN    (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define FB_COLOR_LRED     (FOREGROUND_RED|FOREGROUND_INTENSITY)
#define FB_COLOR_LMAGENTA (FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define FB_COLOR_YELLOW   (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define FB_COLOR_BWHITE   (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)

/* Read the process-local environment maintained by SETENVIRON. */
const char *fb_hWinCEGetEnv( const char *name );

/* Convert non-path UTF-8 or active-code-page text to native UTF-16. */
wchar_t *fb_hWinCEToWC( const char *text );

/* Read the executable path, including the fallback for reduced CE images. */
int fb_hWinCEGetExecutablePathWC( wchar_t *destination,
	                              size_t destination_length );

/* Read the process-local current directory used to resolve relative paths. */
int fb_hWinCEGetCurrentDirectoryWC( wchar_t *destination,
	                                size_t destination_length );

#ifdef ENABLE_MT
	FBCALL void fb_MtLock( void );
	FBCALL void fb_MtUnlock( void );
	#define FB_MTLOCK()   fb_MtLock()
	#define FB_MTUNLOCK() fb_MtUnlock()
#else
	#define FB_MTLOCK()
	#define FB_MTUNLOCK()
#endif

#endif

/* end of wince/fb_wince.h */
