/*
    FreeBASIC runtime Wii thread creation
    -------------------------------------

    File: thread_core.c

    Purpose:

        Adapt FreeBASIC's threadcreate/threadwait runtime hooks to libogc
        lightweight threads.

    Responsibilities:

        - allocate FreeBASIC thread handles
        - create libogc LWP threads
        - set the FreeBASIC TLS self pointer for worker threads
        - release thread bookkeeping after wait or detached exit

    This file intentionally does NOT contain:

        - mutex implementation
        - thread-local storage internals
        - platform scheduler policy
*/

#include "../fb.h"
#include "../fb_private_thread.h"

#define FB_WII_DEFAULT_THREAD_STACK_SIZE (128 * 1024)
#define FB_WII_RETRY_THREAD_STACK_SIZE   (64 * 1024)

static void *threadproc(void *param)
{
	FBTHREADINFO *info = (FBTHREADINFO *)param;
	FBTHREAD *thread = info->thread;
	FBTHREADFLAGS threadFlags;

	FB_TLSGETCTX(FBTHREAD)->self = thread;

	info->proc(info->param);
	free(info);

	fb_TlsFreeCtxTb();
	threadFlags = fb_AtomicSetThreadFlags(&thread->flags, FBTHREAD_EXITED);

	if (threadFlags & FBTHREAD_DETACHED)
		free(thread);

	return (void *)1;
}

FBCALL FBTHREAD *fb_ThreadCreate(FB_THREADPROC proc, void *param, ssize_t stack_size)
{
	FBTHREAD *thread;
	FBTHREADINFO *info;
	u32 real_stack_size;

	thread = (FBTHREAD *)malloc(sizeof(FBTHREAD));
	if (thread == NULL)
		return NULL;

	info = (FBTHREADINFO *)malloc(sizeof(FBTHREADINFO));
	if (info == NULL) {
		free(thread);
		return NULL;
	}

	info->proc = proc;
	info->param = param;
	info->thread = thread;
	thread->flags = FBTHREAD_NONE;

	if (stack_size > 0) {
		real_stack_size = (u32)stack_size;
		if (real_stack_size < (u32)FBTHREAD_STACK_MIN)
			real_stack_size = (u32)FBTHREAD_STACK_MIN;
	} else {
		/*
			libogc exposes an 8 KiB minimum stack, but FreeBASIC threads often
			enter file, string, TCP, and graphics runtime paths.  That minimum
			is too small for ordinary threaded programs.

			The default still has to respect the Wii memory budget.  A stack
			large enough for one TCP worker becomes destructive when a program
			creates many short-lived workers, so keep the default moderate and
			let programs request a larger stack explicitly when they need one.
		*/
		real_stack_size = FB_WII_DEFAULT_THREAD_STACK_SIZE;
	}

	if (LWP_CreateThread(&thread->id, threadproc, info, NULL, real_stack_size, 64) < 0) {
		if ((stack_size <= 0) &&
		    (LWP_CreateThread(&thread->id, threadproc, info, NULL, FB_WII_RETRY_THREAD_STACK_SIZE, 64) >= 0)) {
			return thread;
		}
		free(info);
		free(thread);
		return NULL;
	}

	return thread;
}

FBCALL void fb_ThreadWait(FBTHREAD *thread)
{
	if ((thread == NULL) ||
	    ((thread->flags & (FBTHREAD_MAIN | FBTHREAD_DETACHED)) != 0) ||
	    (thread == FB_TLSGETCTX(FBTHREAD)->self)) {
		return;
	}

	LWP_JoinThread(thread->id, NULL);
	free(thread);
}

/* end of thread_core.c */
