/* thread creation and destruction functions */

#include "../fb.h"
#include "../fb_private_thread.h"
#include <windows.h>

#define FB_XBOX_DEFAULT_THREAD_STACK_SIZE (128 * 1024)
#define FB_XBOX_RETRY_THREAD_STACK_SIZE   (64 * 1024)

static DWORD WINAPI threadproc( void *param )
{
	FBTHREADINFO *info = param;
	FBTHREAD *thread = info->thread;
	FBTHREADFLAGS flags;

	FB_TLSGETCTX( FBTHREAD )->self = thread;

	/* call the user thread procedure */
	info->proc( info->param );
	free( info );

	/* free mem */
	fb_TlsFreeCtxTb( );

	flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_EXITED );

	/* This thread has been detached, we can free the thread structure */
	if( flags & FBTHREAD_DETACHED ) {
		free( thread );
	}

	return 0;
}

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	FBTHREAD *thread;
	FBTHREADINFO *info;
	SIZE_T real_stack_size;

	thread = (FBTHREAD *)malloc( sizeof( FBTHREAD ) );
	if( thread == NULL ) {
		return NULL;
	}

	info = (FBTHREADINFO *)malloc( sizeof( FBTHREADINFO ) );
	if( info == NULL ) {
		free( thread );
		return NULL;
	}

	info->proc = proc;
	info->param = param;
	info->thread = thread;
	thread->flags = FBTHREAD_NONE;
	/*
		Xbox has much less memory than desktop Win32, and nxdk commits the
		stack reserve eagerly enough that creating many 1 MiB stacks can fail
		before user code runs.  Keep an explicit user stack size intact, but
		use a smaller default so ordinary ThreadCreate() fan-out still works.
	*/
	real_stack_size = (stack_size > 0) ? (SIZE_T)stack_size : FB_XBOX_DEFAULT_THREAD_STACK_SIZE;

	thread->id = CreateThread( NULL, real_stack_size, threadproc, info, 0, NULL );
	if( (thread->id == NULL) && (stack_size <= 0) ) {
		thread->id = CreateThread( NULL, FB_XBOX_RETRY_THREAD_STACK_SIZE, threadproc, info, 0, NULL );
	}

	if( thread->id == NULL ) {
		free( thread );
		free( info );
		return NULL;
	}

	return thread;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	/* A wait for the main thread or ourselves will never end
	   also, if we've been detached, we've nothing to wait on
	*/
	if( 
		( thread == NULL ) || 
		( ( thread->flags & ( FBTHREAD_MAIN | FBTHREAD_DETACHED ) ) != 0 ) ||
		( thread == FB_TLSGETCTX( FBTHREAD )->self )
	) {
		return;
	}

	WaitForSingleObject( thread->id, INFINITE );
	CloseHandle( thread->id );
	free( thread );
}
