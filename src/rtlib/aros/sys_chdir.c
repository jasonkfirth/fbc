/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: sys_chdir.c

    Purpose:

        Implement CHDIR through the AROS DOS lock API.

    Responsibilities:

        - normalize FreeBASIC path separators before the DOS lookup
        - suppress volume requesters while probing a possibly absent path
        - replace the process current-directory lock on success
        - release only the lock displaced by the successful replacement
        - preserve the normal FreeBASIC temporary-string lifecycle

    This file intentionally does NOT contain:

        - POSIXC per-library current-directory bookkeeping
        - path rules for other operating systems
        - filesystem creation or removal policy

    AROS note:

        CHDIR is a runtime operation, so it uses dos.library directly.  The
        POSIXC chdir() wrapper owns private cd_changed state whose library
        teardown can contend with FreeBASIC runtime shutdown on hosted AROS
        filesystems.  CurrentDir() is the native process-wide operation and
        has the ownership rules needed here.

        DOS normally opens an interactive volume requester when Lock() sees
        an unknown volume name.  CHDIR must report failure to the BASIC
        program instead of interrupting it, so the process requester pointer
        is temporarily set to the documented no-requester sentinel.  The
        original pointer is restored before this function examines the lock.
*/

#include "../fb.h"

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* ------------------------------------------------------------------------- */
/* Current-directory replacement                                             */
/* ------------------------------------------------------------------------- */

FBCALL int fb_ChDir( FBSTRING *path )
{
	BPTR new_lock;
	BPTR old_lock;
	char *converted_path;
	int result;
	struct Process *process;
	APTR window_pointer;

	result = -1;
	converted_path = NULL;
	if( (path != NULL) && (path->data != NULL) )
	{
		converted_path = malloc( strlen( path->data ) + 1 );
		if( converted_path != NULL )
		{
			strcpy( converted_path, path->data );
			fb_hConvertPath( converted_path );
			process = (struct Process *)FindTask( NULL );
			if( process != NULL )
			{
				window_pointer = process->pr_WindowPtr;
				process->pr_WindowPtr = (APTR)-1;
			}
			new_lock = Lock( (CONST_STRPTR)converted_path, SHARED_LOCK );
			if( process != NULL )
				process->pr_WindowPtr = window_pointer;
			if( new_lock != BNULL )
			{
				old_lock = CurrentDir( new_lock );
				if( old_lock != BNULL )
					UnLock( old_lock );
				result = 0;
			}
		}
	}

	free( converted_path );
	fb_hStrDelTemp( path );
	return result;
}

/* end of sys_chdir.c */
