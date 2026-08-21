/*
    FreeBASIC runtime library
    -------------------------

    File: aros/thread_core.c

    Purpose:

        Implement FreeBASIC thread creation and waiting on AROS.

    Responsibilities:

        - create FreeBASIC threads through the AROS pthread library
        - establish each child thread's FreeBASIC thread-local identity
        - arbitrate the AROS exit-versus-detach race
        - wait for joinable threads and release their handles

    This file intentionally does NOT contain:

        - generic Unix thread policy
        - an AROS pthread implementation
        - architecture-specific scheduling policy

    The AROS pthread implementation cannot clean up a thread that is detached
    after it has already exited. Its process-exit handler then waits forever
    on the stale thread slot. FreeBASIC's flags provide the synchronization
    point needed to avoid that state: a child that observes an earlier detach
    request detaches itself before returning, while the parent joins a child
    whose exit flag was already visible. The atomic flag update ensures that
    exactly one side owns the native cleanup operation.
*/

#include "../fb.h"
#include "../fb_private_thread.h"

/* ------------------------------------------------------------------------- */
/* Child thread proxy                                                        */
/* ------------------------------------------------------------------------- */

static void *threadproc( void *param )
{
	static int thread_result;
	FBTHREADINFO *info = param;
	FBTHREAD *thread = info->thread;
	FBTHREADFLAGS thread_flags;

	FB_TLSGETCTX( FBTHREAD )->self = thread;

	info->proc( info->param );

	free( info );
	fb_TlsFreeCtxTb( );

	thread_flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_EXITED );
	if( thread_flags & FBTHREAD_DETACHED ) {
		/* AROS must see the detach before StarterFunc records completion. */
		pthread_detach( pthread_self( ) );
		free( thread );
	}

	/* AROS calls exit() when a pthread start routine returns NULL. */
	return &thread_result;
}

/* ------------------------------------------------------------------------- */
/* Public thread operations                                                  */
/* ------------------------------------------------------------------------- */

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	FBTHREAD *thread;
	FBTHREADINFO *info;
	pthread_attr_t thread_attributes;

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

	if( pthread_attr_init( &thread_attributes ) ) {
		free( thread );
		free( info );
		return NULL;
	}

	stack_size = stack_size >= (ssize_t)FBTHREAD_STACK_MIN
		? stack_size
		: (ssize_t)FBTHREAD_STACK_MIN;
	pthread_attr_setstacksize( &thread_attributes, stack_size );

	if( pthread_create( &thread->id, &thread_attributes, threadproc, info ) ) {
		free( thread );
		free( info );
		thread = NULL;
	}

	pthread_attr_destroy( &thread_attributes );
	return thread;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	if(
		( thread == NULL ) ||
		( ( thread->flags & ( FBTHREAD_MAIN | FBTHREAD_DETACHED ) ) != 0 ) ||
		( thread == FB_TLSGETCTX( FBTHREAD )->self )
	) {
		return;
	}

	pthread_join( thread->id, NULL );
	free( thread );
}

/* end of aros/thread_core.c */
