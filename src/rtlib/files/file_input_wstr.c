/*
 * Project: FreeBASIC runtime library
 * File: files/file_input_wstr.c
 * Purpose: input function for wstring's.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_InputWstr( FB_WCHAR *str, ssize_t length )
{
	FB_INPUTCTX *ctx = FB_TLSGETCTX( INPUT );
	if( ctx == NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	if( !FB_HANDLE_USED( ctx->handle ) ||
	    (FB_HANDLE_IS_SCREEN( ctx->handle ) && fb_IsRedirected( TRUE )) ) {
		/* Console records are bytes even when the destination is WSTRING.
		   Scan delimiters in that byte stream so narrow and wide destinations
		   share one position, then decode the complete field. */
		ssize_t capacity = length;
		ssize_t max_bytes;
		char local_token[256];
		char *token;
		int bytes;
		if( capacity <= 0 || capacity > FB_INPUT_MAXSTRINGLEN + 1 )
			capacity = FB_INPUT_MAXSTRINGLEN + 1;
		max_bytes = 4 * capacity; /* longest UTF-8 scalar */
		token = (size_t)max_bytes < sizeof(local_token) ? local_token :
		        (char *)malloc( (size_t)max_bytes + 1 );
		if( token == NULL )
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		bytes = fb_hConsoleInputNextToken( token, max_bytes );
		fb_hConsoleBytesToWstr( str, capacity, token, bytes );
		if( token != local_token )
			free( token );
	} else {
		FB_WCHAR buffer[FB_INPUT_MAXSTRINGLEN + 1];
		if( FB_HANDLE_IS_SCREEN( ctx->handle ) )
			fb_DevScrnInit_ReadWstr();
		fb_FileInputNextTokenWstr( buffer, FB_INPUT_MAXSTRINGLEN, TRUE );
		fb_WstrAssign( str, length, buffer );
	}
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of files/file_input_wstr.c */
