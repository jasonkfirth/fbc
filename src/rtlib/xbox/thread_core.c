/* thread creation and destruction functions */

#include "../fb.h"
#include "../fb_private_thread.h"
#include <windows.h>

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
	thread->flags = 0;
	real_stack_size = (stack_size > 0) ? (SIZE_T)stack_size : 65536;

	thread->id = CreateThread( NULL, real_stack_size, threadproc, info, 0, NULL );
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
