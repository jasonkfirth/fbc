/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/thread_core.c

    Purpose:

        Implement FreeBASIC thread creation and joining on Windows CE.

    Responsibilities:

        - launch user procedures through CreateThread
        - establish and tear down per-thread runtime state
        - coordinate detached and joined thread-object ownership

    This file intentionally does NOT contain:

        - desktop CRT _beginthreadex integration
        - condition variables or mutexes
        - thread scheduling policy
*/

#include "../fb.h"
#include "../fb_private_thread.h"

#include <windows.h>

static DWORD WINAPI hThreadProcedure( LPVOID parameter )
{
	FBTHREADINFO *info = (FBTHREADINFO *)parameter;
	FBTHREAD *thread = info->thread;
	FBTHREADFLAGS flags;

	FB_TLSGETCTX( FBTHREAD )->self = thread;

	info->proc( info->param );
	free( info );

	fb_TlsFreeCtxTb();

	flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_EXITED );
	if( (flags & FBTHREAD_DETACHED) != 0 )
		free( thread );

	return 1;
}

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC procedure, void *parameter,
	                              ssize_t stack_size )
{
	FBTHREAD *thread;
	FBTHREADINFO *info;
	DWORD thread_id;
	SIZE_T native_stack_size;

	if( procedure == NULL )
		return NULL;

	thread = (FBTHREAD *)malloc( sizeof( FBTHREAD ) );
	if( thread == NULL )
		return NULL;

	info = (FBTHREADINFO *)malloc( sizeof( FBTHREADINFO ) );
	if( info == NULL ) {
		free( thread );
		return NULL;
	}

	info->proc = procedure;
	info->param = parameter;
	info->thread = thread;
	thread->flags = FBTHREAD_NONE;

	native_stack_size = (stack_size > 0) ? (SIZE_T)stack_size : 0;
	thread->id = CreateThread( NULL, native_stack_size, hThreadProcedure,
	                           info, 0, &thread_id );
	if( thread->id == NULL ) {
		free( info );
		free( thread );
		return NULL;
	}

	return thread;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	if( (thread == NULL) ||
	    ((thread->flags & (FBTHREAD_MAIN | FBTHREAD_DETACHED)) != 0) ||
	    (thread == FB_TLSGETCTX( FBTHREAD )->self) ) {
		return;
	}

	WaitForSingleObject( thread->id, INFINITE );
	CloseHandle( thread->id );
	free( thread );
}

/* end of wince/thread_core.c */
