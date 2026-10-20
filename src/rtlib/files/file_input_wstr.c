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
	FB_WCHAR buffer[FB_INPUT_MAXSTRINGLEN+1];

	fb_FileInputNextTokenWstr( buffer, FB_INPUT_MAXSTRINGLEN, TRUE );

	fb_WstrAssign( str, length, buffer );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of files/file_input_wstr.c */
