/*
 * Project: FreeBASIC runtime library
 * File: files/file_print_wstr.c
 * Purpose: print # function (formating is done at io_prn).
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
int fb_hFilePrintBufferWstrEx
	(
		FB_FILE *handle,
		const FB_WCHAR *buffer,
		size_t len
	)
{
    fb_DevScrnInit_WriteWstr( );
	return fb_FilePutDataEx( handle, 0, buffer, len, TRUE, TRUE, TRUE );
}

/*:::::*/
int fb_hFilePrintBufferWstr
	(
		int fnum,
		const FB_WCHAR *buffer
	)
{
    return fb_hFilePrintBufferWstrEx( FB_FILE_TO_HANDLE(fnum),
                                  	  buffer, fb_wstr_Len( buffer ) );
}

/* end of files/file_print_wstr.c */
