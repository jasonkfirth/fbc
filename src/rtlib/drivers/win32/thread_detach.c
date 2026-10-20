/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/thread_detach.c
 * Purpose: Provide thread detach runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

FBCALL void fb_ThreadDetach( FBTHREAD *thread )
{
	FBTHREADFLAGS flags;

	if( thread == NULL || ( thread->flags & FBTHREAD_MAIN ) )
		return;

	flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_DETACHED );
#ifdef HOST_CYGWIN
	pthread_detach( thread->id );
#else
	CloseHandle( thread->id );
#endif

	if( flags & FBTHREAD_EXITED ) {
		free( thread );
	}
}

/* end of drivers/win32/thread_detach.c */
