/*
    FreeBASIC runtime library
    -------------------------

    File: amiga/thread_detach.c

    Purpose:

        Detach FreeBASIC threads without leaking AmigaOS pthread slots.

    Responsibilities:

        - publish a detach request atomically to the child thread
        - reap a child whose FreeBASIC exit flag is already visible
        - leave a running child responsible for native self-detachment

    This file intentionally does NOT contain:

        - generic Unix detach behavior
        - thread creation or waiting
        - changes to the AmigaOS pthread implementation

    See amiga/thread_core.c for the two-sided exit-versus-detach protocol.
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
		/* Join only to release AmigaOS's finished slot; the API stays detached. */
		pthread_join( thread->id, NULL );
		free( thread );
	}
}

/* end of amiga/thread_detach.c */
