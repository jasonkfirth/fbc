/*
    FreeBASIC runtime library
    -------------------------

    File: aros/thread_detach.c

    Purpose:

        Detach FreeBASIC threads without leaking AROS pthread slots.

    Responsibilities:

        - publish a detach request atomically to the child thread
        - reap a child whose FreeBASIC exit flag is already visible
        - leave a running child responsible for native self-detachment

    This file intentionally does NOT contain:

        - generic Unix detach behavior
        - thread creation or waiting
        - changes to the AROS pthread implementation

    See aros/thread_core.c for the two-sided exit-versus-detach protocol.
*/

#include "../fb.h"
#include "../fb_private_thread.h"

/* ------------------------------------------------------------------------- */
/* Public detach operation                                                   */
/* ------------------------------------------------------------------------- */

FBCALL void fb_ThreadDetach( FBTHREAD *thread )
{
	FBTHREADFLAGS thread_flags;

	if( thread == NULL || ( thread->flags & FBTHREAD_MAIN ) ) {
		return;
	}

	thread_flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_DETACHED );
	if( thread_flags & FBTHREAD_EXITED ) {
		/* Join only to release AROS's finished slot; the API stays detached. */
		pthread_join( thread->id, NULL );
		free( thread );
	}
}

/* end of aros/thread_detach.c */
