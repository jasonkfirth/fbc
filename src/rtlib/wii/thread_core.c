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

	if (stack_size < (ssize_t)FBTHREAD_STACK_MIN)
		stack_size = (ssize_t)FBTHREAD_STACK_MIN;

	if (LWP_CreateThread(&thread->id, threadproc, info, NULL, (u32)stack_size, 64) < 0) {
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
