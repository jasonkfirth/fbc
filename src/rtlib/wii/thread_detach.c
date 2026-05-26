/*
    FreeBASIC runtime Wii thread detach
    -----------------------------------

    File: thread_detach.c

    Purpose:

        Mark Wii LWP-backed FreeBASIC threads as detached.

    Responsibilities:

        - preserve FreeBASIC's detached-thread ownership rules
        - free bookkeeping immediately if the thread has already exited

    This file intentionally does NOT contain:

        - thread creation
        - thread joining
        - mutex implementation
*/

#include "../fb.h"
#include "../fb_private_thread.h"

FBCALL void fb_ThreadDetach(FBTHREAD *thread)
{
	FBTHREADFLAGS flags;

	if ((thread == NULL) || (thread->flags & FBTHREAD_MAIN))
		return;

	flags = fb_AtomicSetThreadFlags(&thread->flags, FBTHREAD_DETACHED);

	if (flags & FBTHREAD_EXITED)
		free(thread);
}

/* end of thread_detach.c */
