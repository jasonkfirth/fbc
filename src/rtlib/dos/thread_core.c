/* FreeBASIC DOS runtime: thread_core.c
 *
 * Own thread handles, TLS tables, and lifecycle for the opt-in RTC scheduler.
 * PDMLWP releases exited worker stacks from its cleanup thread. A FreeBASIC
 * handle survives until join/detach; TLS is destroyed by its owning worker.
 * The ordinary DOS archive retains unsupported-operation stubs.
 */

#include "../fb.h"
#include "../fb_private_thread.h"

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

typedef struct {
	void *slots[FB_TLSKEYS];
} FB_DOS_TLS;

static FB_DOS_TLS main_tls;
static int scheduler_ready;
static int scheduler_stopped;

int fb_DosThreadInit( void )
{
	int previous = fb_DosThreadEnter();
	if( scheduler_stopped ) {
		fb_DosThreadLeave( previous );
		return 0;
	}
	if( !scheduler_ready ) {
		/* IRQ8 at 128 Hz leaves IRQ0/PIT timing used by graphics untouched. */
		if( !lwp_init( 8, RTC128 ) ) {
			fb_DosThreadLeave( previous );
			return 0;
		}
		lwp_thread_disable();
		lwp_setuserptr( &main_tls );
		scheduler_ready = 1;
		previous = fb_DosThreadStarted();
	}
	fb_DosThreadLeave( previous );
	return 1;
}

void *fb_DosTlsGet( int index )
{
	FB_DOS_TLS *tls = scheduler_ready && _lwp_cur ? lwp_getuserptr() : &main_tls;
	return tls && index >= 0 && index < FB_TLSKEYS ? tls->slots[index] : NULL;
}

int fb_DosTlsSet( int index, void *value )
{
	FB_DOS_TLS *tls = scheduler_ready && _lwp_cur ? lwp_getuserptr() : &main_tls;
	if( !tls || index < 0 || index >= FB_TLSKEYS )
		return -1;
	tls->slots[index] = value;
	return 0;
}

void fb_DosThreadExit( void )
{
	/* Runtime teardown must no longer admit involuntary worker execution. */
	lwp_thread_disable();
	scheduler_stopped = 1;
}

static void threadproc( void *parameter )
{
	FBTHREADINFO *info = parameter;
	FBTHREAD *thread = info->thread;
	FB_DOS_TLS *tls = thread->opaque;
	int previous;

	lwp_setuserptr( tls );
	info->proc( info->param );
	free( info );
	fb_TlsFreeCtxTb();

	previous = fb_DosThreadEnter();
	thread->opaque = NULL;
	lwp_setuserptr( NULL );
	free( tls );
	thread->flags |= FBTHREAD_EXITED;
	if( thread->flags & FBTHREAD_DETACHED )
		free( thread );
	fb_DosThreadLeave( previous );
	/* Return through PDMLWP's exit trampoline; never free our own stack. */
}

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	FBTHREAD *thread;
	FBTHREADINFO *info;
	FB_TLS_CTX_HEADER *self_header;
	int previous;

	if( !proc || stack_size < 0 || stack_size > INT_MAX - 31 || !fb_DosThreadInit() )
		return NULL;
	/* Mixing, string conversion, and TLS destructors need more than the old
	 * scheduler's 4 KiB example stacks. Explicit larger requests are honored.
	 */
	if( stack_size < 256 * 1024 )
		stack_size = 256 * 1024;
	previous = fb_DosThreadEnter();
	thread = calloc( 1, sizeof( *thread ) );
	info = malloc( sizeof( *info ) );
	if( !thread || !info ) {
		free( thread );
		free( info );
		fb_DosThreadLeave( previous );
		return NULL;
	}
	thread->opaque = calloc( 1, sizeof( FB_DOS_TLS ) );
	/* Allocate the initial ThreadSelf context before publishing the worker.
	 * An exhausted heap must fail ThreadCreate, not crash a new thread.
	 */
	self_header = calloc( 1, sizeof( *self_header ) + sizeof( FB_FBTHREADCTX ) );
	if( thread->opaque && self_header ) {
		FB_DOS_TLS *tls = thread->opaque;
		FB_FBTHREADCTX *self = (FB_FBTHREADCTX *)(self_header + 1);
		self->self = thread;
		tls->slots[FB_TLSKEY_FBTHREAD] = self;
	}
	info->proc = proc;
	info->param = param;
	info->thread = thread;
	thread->id = thread->opaque && self_header ? lwp_spawn( threadproc, info, stack_size, 1 ) : -1;
	if( thread->id < 0 ) {
		free( self_header );
		free( thread->opaque );
		free( thread );
		free( info );
		thread = NULL;
	}
	fb_DosThreadLeave( previous );
	return thread;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	if( !thread || (thread->flags & (FBTHREAD_MAIN | FBTHREAD_DETACHED)) ||
	    thread == fb_ThreadSelf() )
		return;
	while( !(thread->flags & FBTHREAD_EXITED) )
		lwp_yield();
	free( thread );
}

FBCALL void fb_ThreadDetach( FBTHREAD *thread )
{
	int previous;
	if( !thread )
		return;
	previous = fb_DosThreadEnter();
	if( !(thread->flags & (FBTHREAD_MAIN | FBTHREAD_DETACHED)) ) {
		thread->flags |= FBTHREAD_DETACHED;
		if( thread->flags & FBTHREAD_EXITED )
			free( thread );
	}
	fb_DosThreadLeave( previous );
}

#else

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	(void)proc;
	(void)param;
	(void)stack_size;
	return NULL;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	(void)thread;
}

FBCALL void fb_ThreadDetach( FBTHREAD *thread )
{
	(void)thread;
}

#endif

/* end of thread_core.c */
