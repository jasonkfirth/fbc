/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_error.c

    Purpose:

        Provide the small error-reporting entry points that generated
        FreeBASIC programs expect when assertions are enabled on NuttX.

    Responsibilities:

        - implement ASSERT failure reporting
        - implement ASSERTWARN reporting
        - provide narrow and wide expression entry points
        - terminate the current BASIC program after a hard assertion failure

    This file intentionally does NOT contain:

        - general runtime error dispatch
        - ON ERROR / RESUME state management
        - console input handling
        - NuttX process startup or shutdown logic
*/

#include "fb.h"

#include <stdint.h>

#ifndef FBCALL
#define FBCALL
#endif

#ifndef FB_WCHAR
typedef uint32_t FB_WCHAR;
#endif

extern void fb_End( const int32 status );

#if !defined(FB_NUTTX_USE_GENERIC_ASSERT) || \
    (FB_NUTTX_USE_GENERIC_ASSERT == 0)

/* ------------------------------------------------------------------------- */
/* Assertion message helpers                                                 */
/* ------------------------------------------------------------------------- */

static const char *fb_nuttx_safe_cstr( const char *text )
{
	if( text != NULL )
		return text;

	return "(null)";
}

static void fb_nuttx_print_assert_header( const char *filename, int linenum,
                                          const char *funcname )
{
	printf( "%s(%d): assertion failed at %s: ",
	         fb_nuttx_safe_cstr( filename ),
	         linenum,
	         fb_nuttx_safe_cstr( funcname ) );
}

static void fb_nuttx_print_wstr_lossy( const FB_WCHAR *text )
{
	size_t length;

	if( text == NULL ) {
		fputs( "(null)", stdout );
		return;
	}

	/*
	    Assertion expressions are compiler-generated diagnostic text.  Keeping
	    this path allocation-free matters more than preserving every Unicode
	    code point on tiny NuttX boards, so non-ASCII characters are shown as
	    '?' instead of building a temporary converted string.
	*/
	for( length = 0; (text[length] != 0) && (length < 1024); length++ ) {
		FB_WCHAR ch = text[length];

		if( (ch >= 32) && (ch <= 126) )
			fputc( (int)ch, stdout );
		else
			fputc( '?', stdout );
	}

	if( text[length] != 0 )
		fputs( "...", stdout );
}

static void fb_nuttx_finish_assert_failure( void )
{
	fputc( '\n', stdout );
	fflush( stdout );

	/*
	    A failed ASSERT is fatal in the normal runtime.  NuttX apps are run by
	    the test harness as small command-style programs, so ending the BASIC
	    program with a non-zero status gives fbctests the same contract.
	*/
	fb_End( 1 );
}

/* ------------------------------------------------------------------------- */
/* Public assertion entry points                                             */
/* ------------------------------------------------------------------------- */

FBCALL void fb_Assert( char *filename, int linenum, char *funcname, char *expression )
{
	fb_nuttx_print_assert_header( filename, linenum, funcname );
	fputs( fb_nuttx_safe_cstr( expression ), stdout );
	fb_nuttx_finish_assert_failure();
}

FBCALL void fb_AssertWarn( char *filename, int linenum, char *funcname, char *expression )
{
	fb_nuttx_print_assert_header( filename, linenum, funcname );
	fputs( fb_nuttx_safe_cstr( expression ), stdout );
	fputc( '\n', stdout );
	fflush( stdout );
}

FBCALL void fb_AssertW( char *filename, int linenum, char *funcname, FB_WCHAR *expression )
{
	fb_nuttx_print_assert_header( filename, linenum, funcname );
	fb_nuttx_print_wstr_lossy( expression );
	fb_nuttx_finish_assert_failure();
}

FBCALL void fb_AssertWarnW( char *filename, int linenum, char *funcname, FB_WCHAR *expression )
{
	fb_nuttx_print_assert_header( filename, linenum, funcname );
	fb_nuttx_print_wstr_lossy( expression );
	fputc( '\n', stdout );
	fflush( stdout );
}

#endif

/* end of fb_nuttx_error.c */
