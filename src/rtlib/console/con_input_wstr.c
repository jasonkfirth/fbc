/*
 * Project: FreeBASIC runtime library
 * File: console/con_input_wstr.c
 * Purpose: Start a console INPUT statement with a WSTRING destination.
 * Responsibilities: Preserve the input record and the shared token cursor.
 * This file intentionally does NOT contain: Token parsing or file decoding.
 */
#include "fb.h"

FBCALL int fb_ConsoleInputWstr( FBSTRING_ACCESS *text, int addquestion, int addnewline )
{
	FB_INPUTCTX *ctx;

	/* For redirected input, leave the cursor on the stream. Other fields
	   and later INPUT statements may still need the bytes on this line. */
	fb_DevScrnInit_Read();
	if( fb_IsRedirected( TRUE ) ) {
		fb_hStrDelTemp( text );
		return fb_FileInput( 0 );
	}

	ctx = FB_TLSGETCTX( INPUT );
	if( ctx == NULL ) {
		fb_hStrDelTemp( text );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}
	fb_StrDelete( &ctx->str );
	ctx->handle = NULL;
	ctx->status = 0;
	ctx->index = 0;
	return fb_hConsoleLineInputUtf8( text, &ctx->str, addquestion, addnewline );
}

/* end of console/con_input_wstr.c */
