/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_execex.c

    Purpose:

        Launch another program through the Windows CE process API.

    Responsibilities:

        - convert executable paths and command lines to UTF-16
        - support both waiting EXEC and non-waiting RUN behavior
        - release process handles and temporary FreeBASIC strings

    This file intentionally does NOT contain:

        - desktop CRT spawn or exec calls
        - command-interpreter policy
        - shell verb dispatch
*/

#include "../fb.h"

#include <windows.h>

FBCALL int fb_ExecEx( FBSTRING *program, FBSTRING *args, int do_fork )
{
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;
	wchar_t *wide_program = NULL;
	wchar_t *wide_arguments = NULL;
	DWORD exit_code;
	int result = -1;

	if( (program == NULL) || (program->data == NULL) )
		goto cleanup;

	wide_program = fb_hConvertPathToWC( program->data, NULL );
	if( wide_program == NULL )
		goto cleanup;

	if( (args != NULL) && (args->data != NULL) ) {
		wide_arguments = fb_hWinCEToWC( args->data );
		if( wide_arguments == NULL )
			goto cleanup;
	}

	memset( &startup_info, 0, sizeof( startup_info ) );
	memset( &process_info, 0, sizeof( process_info ) );
	startup_info.cb = sizeof( startup_info );

	if( !CreateProcessW( wide_program, wide_arguments, NULL, NULL, FALSE, 0,
	                     NULL, NULL, &startup_info, &process_info ) ) {
		goto cleanup;
	}

	if( process_info.hThread != NULL )
		CloseHandle( process_info.hThread );

	if( do_fork ) {
		if( (WaitForSingleObject( process_info.hProcess, INFINITE ) ==
		     WAIT_OBJECT_0) &&
		    GetExitCodeProcess( process_info.hProcess, &exit_code ) ) {
			result = (int)exit_code;
		}
		CloseHandle( process_info.hProcess );
	} else {
		result = (int)(intptr_t)process_info.hProcess;
	}

cleanup:
	free( wide_arguments );
	free( wide_program );

	FB_STRLOCK();
	fb_hStrDelTemp_NoLock( args );
	fb_hStrDelTemp_NoLock( program );
	FB_STRUNLOCK();

	return result;
}

/* end of wince/sys_execex.c */
