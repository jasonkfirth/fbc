/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_execex.c
 * Purpose: Provide sys execex runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <process.h>

FBCALL int fb_ExecEx( FBSTRING_ACCESS *program, FBSTRING_ACCESS *args, int do_fork )
{
    char buffer[MAX_PATH+1], *arguments;
    int	res = 0, got_program;
    int arguments_heap_allocated = FALSE;
    size_t len_arguments;
#ifdef HOST_MINGW
    char empty_arguments[] = "";
#else
    size_t len_program;
#endif

    got_program = (program != NULL) && (program->data != NULL);

    if( !got_program ) {
        fb_hStrDelTemp( args );
        fb_hStrDelTemp( program );
        return -1;
    }

    fb_hGetShortPath( program->data, buffer, MAX_PATH );

#ifdef HOST_MINGW
    if( args==NULL ) {
        arguments = empty_arguments;
    } else {
        len_arguments = FB_STRSIZE( args );
        if( len_arguments >= (size_t)PTRDIFF_MAX ) {
            fb_hStrDelTemp( args );
            fb_hStrDelTemp( program );
            return -1;
        }
        arguments = (char *)malloc( len_arguments + 1 );
        if( arguments == NULL ) {
            fb_hStrDelTemp( args );
            fb_hStrDelTemp( program );
            return -1;
        }
        arguments_heap_allocated = TRUE;
        if( len_arguments )
            FB_MEMCPY( arguments, args->data, len_arguments );
        arguments[len_arguments] = 0;
    }
#else
    len_program = strlen( buffer );
    len_arguments = ( ( args==NULL ) ? 0 : FB_STRSIZE( args ) );

    if( (len_program > (size_t)PTRDIFF_MAX - 2u) ||
        (len_arguments > (size_t)PTRDIFF_MAX - len_program - 2u) ) {
        fb_hStrDelTemp( args );
        fb_hStrDelTemp( program );
        return -1;
    }
    arguments = (char *)malloc( len_program + len_arguments + 2u );
    if( arguments == NULL ) {
        fb_hStrDelTemp( args );
        fb_hStrDelTemp( program );
        return -1;
    }
    arguments_heap_allocated = TRUE;

    FB_MEMCPY( arguments, buffer, len_program );
    arguments[len_program] = ' ';
    if( len_arguments!=0 )
        FB_MEMCPY( arguments + len_program + 1, args->data, len_arguments );
    arguments[len_program + len_arguments + 1] = 0;
#endif

	FB_STRLOCK();

	fb_hStrDelTemp_NoLock( args );
    fb_hStrDelTemp_NoLock( program );

    FB_STRUNLOCK();

    FB_CON_CORRECT_POSITION();

	{
#ifdef HOST_MINGW
        if( do_fork )
            res = (int)_spawnl( _P_WAIT, buffer, buffer, arguments, NULL );
        else
            res = (int)_execl( buffer, buffer, arguments, NULL );
#else
        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInfo;
        memset( &StartupInfo, 0, sizeof(StartupInfo) );
        StartupInfo.cb = sizeof(StartupInfo);

        if( !CreateProcess( NULL,        /* application name - correct! */
                            arguments,   /* command line */
                            NULL, NULL,  /* default security descriptors */
                            FALSE,       /* don't inherit handles */
                            CREATE_DEFAULT_ERROR_MODE, /* do we really need this? */
                            NULL,        /* default environment */
                            NULL,        /* current directory */
                            &StartupInfo,
                            &ProcessInfo ) )
        {
            res = -1;
        } else {
            /* Release main thread handle - we're not interested in it */
            CloseHandle( ProcessInfo.hThread );
            if( do_fork ) {
                DWORD dwExitCode;
                WaitForSingleObject( ProcessInfo.hProcess,
                                     INFINITE );
                if( !GetExitCodeProcess( ProcessInfo.hProcess, &dwExitCode ) ) {
                    res = -1;
                } else {
                    res = (int) dwExitCode;
                }
                CloseHandle( ProcessInfo.hProcess );
            } else {
                res = (int)(intptr_t)ProcessInfo.hProcess;
            }
        }
#endif
	}

	if( arguments_heap_allocated )
		free( arguments );

	return res;
}

/* end of drivers/win32/sys_execex.c */
