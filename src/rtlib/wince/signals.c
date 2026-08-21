/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/signals.c

    Purpose:

        Translate fatal Windows CE structured exceptions into FreeBASIC
        runtime errors.

    Responsibilities:

        - install one process-wide unhandled-exception filter
        - map stable CPU exception classes to FreeBASIC error numbers
        - preserve an earlier exception filter for unrecognized failures

    This file intentionally does NOT contain:

        - POSIX signal emulation
        - console control handling
        - recoverable hardware exception support
*/

#include "../fb.h"

#include <windows.h>

static LPTOP_LEVEL_EXCEPTION_FILTER fb_wince_previous_exception_filter;

/* ------------------------------------------------------------------------- */
/* Exception translation                                                     */
/* ------------------------------------------------------------------------- */

static int fb_hWinceExceptionError( DWORD exception_code )
{
	switch( exception_code ) {
	case EXCEPTION_ACCESS_VIOLATION:
	case EXCEPTION_STACK_OVERFLOW:
		return FB_RTERROR_SIGSEGV;

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_OVERFLOW:
		return FB_RTERROR_SIGFPE;

	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return FB_RTERROR_SIGILL;

	default:
		return FB_RTERROR_OK;
	}
}

static LONG WINAPI fb_hWinceExceptionFilter( LPEXCEPTION_POINTERS information )
{
	FB_ERRHANDLER handler;
	int error;

	if( information == NULL || information->ExceptionRecord == NULL )
		return EXCEPTION_CONTINUE_SEARCH;

	error = fb_hWinceExceptionError( information->ExceptionRecord->ExceptionCode );
	if( error == FB_RTERROR_OK ) {
		if( fb_wince_previous_exception_filter != NULL )
			return fb_wince_previous_exception_filter( information );
		return EXCEPTION_CONTINUE_SEARCH;
	}

	handler = fb_ErrorThrowEx( error, -1, NULL, NULL, NULL );
	if( handler != NULL )
		handler();

	fb_End( error );
	return EXCEPTION_EXECUTE_HANDLER;
}

FBCALL void fb_InitSignals( void )
{
	fb_wince_previous_exception_filter =
		SetUnhandledExceptionFilter( fb_hWinceExceptionFilter );
}

/* end of wince/signals.c */
