/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/amiga/thread_core.c

    Purpose:

        Implement FreeBASIC thread creation and waiting on AmigaOS.

    Responsibilities:

        - create FreeBASIC threads through the AmigaOS pthread library
        - establish each child thread's FreeBASIC thread-local identity
        - inherit native standard streams without transferring handle ownership
        - arbitrate the AmigaOS exit-versus-detach race
        - wait for joinable threads and release their handles

    This file intentionally does NOT contain: - generic Unix thread policy
        - an AmigaOS pthread implementation
        - architecture-specific scheduling policy

    The AmigaOS pthread implementation cannot clean up a thread that is detached
    after it has already exited. Its process-exit handler then waits forever
    on the stale thread slot. FreeBASIC's flags provide the synchronization
    point needed to avoid that state: a child that observes an earlier detach
    request detaches itself before returning, while the parent joins a child
    whose exit flag was already visible. The atomic flag update ensures that
    exactly one side owns the native cleanup operation.
*/

#include "../../fb.h"
#include "../../fb_private_thread.h"

#include <dos/dos.h>
#include <proto/dos.h>

/* The pthread provider creates DOS processes with private standard streams.
   Capture the creator's handles before starting the child. They are borrowed:
   restore the provider's handles before it performs process-exit cleanup, so
   that cleanup cannot close streams still in use by the parent or siblings. */
typedef struct {
	FB_THREADPROC proc;
	void *param;
	FBTHREAD *thread;
	BPTR input;
	BPTR output;
} FB_AMIGA_THREADINFO;

/* ------------------------------------------------------------------------- */
/* Child thread proxy                                                        */
/* ------------------------------------------------------------------------- */

static void *threadproc( void *param )
{
	static int thread_result;
	FB_AMIGA_THREADINFO *info = param;
	FBTHREAD *thread = info->thread;
	FBTHREADFLAGS thread_flags;
	BPTR previous_input = SelectInput( info->input );
	BPTR previous_output = SelectOutput( info->output );

	FB_TLSGETCTX( FBTHREAD )->self = thread;

	info->proc( info->param );

	free( info );
	fb_TlsFreeCtxTb( );
	SelectInput( previous_input );
	SelectOutput( previous_output );

	thread_flags = fb_AtomicSetThreadFlags( &thread->flags, FBTHREAD_EXITED );
	if( thread_flags & FBTHREAD_DETACHED ) {
		/* AmigaOS must see the detach before StarterFunc records completion. */
		pthread_detach( pthread_self( ) );
		free( thread );
	}

	/* AmigaOS calls exit() when a pthread start routine returns NULL. */
	return &thread_result;
}

/* ------------------------------------------------------------------------- */
/* Public thread operations                                                  */
/* ------------------------------------------------------------------------- */

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	FBTHREAD *thread;
	FB_AMIGA_THREADINFO *info;
	pthread_attr_t thread_attributes;

	thread = (FBTHREAD *)malloc( sizeof( FBTHREAD ) );
	if( thread == NULL ) {
		return NULL;
	}

	info = (FB_AMIGA_THREADINFO *)malloc( sizeof( FB_AMIGA_THREADINFO ) );
	if( info == NULL ) {
		free( thread );
		return NULL;
	}

	info->proc = proc;
	info->param = param;
	info->thread = thread;
	info->input = Input( );
	info->output = Output( );
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

/* end of drivers/amiga/thread_core.c */
