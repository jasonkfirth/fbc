/* FreeBASIC DOS runtime: thread_cond.c
 * Queued condition waits for the optional RTC scheduler. Enqueue and mutex
 * release form one operation with scheduling excluded, preventing a lost
 * signal. Waiter records live on locked worker stacks until signaled. This
 * file does not provide timed waits or asynchronous thread cancellation.
 */

#include "../fb.h"
#include "../fb_private_thread.h"

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

typedef struct FB_DOS_WAITER {
	struct FB_DOS_WAITER *next;
	volatile int signaled;
} FB_DOS_WAITER;

struct _FBCOND {
	FB_DOS_WAITER *head;
	FB_DOS_WAITER *tail;
};

FBCALL FBCOND *fb_CondCreate( void )
{
	return calloc( 1, sizeof( FBCOND ) );
}

FBCALL void fb_CondDestroy( FBCOND *cond )
{
	if( cond && !cond->head )
		free( cond );
}

FBCALL void fb_CondSignal( FBCOND *cond )
{
	int previous = fb_DosThreadEnter();
	if( cond && cond->head ) {
		FB_DOS_WAITER *waiter = cond->head;
		cond->head = waiter->next;
		if( !cond->head )
			cond->tail = NULL;
		waiter->signaled = 1;
	}
	fb_DosThreadLeave( previous );
}

FBCALL void fb_CondBroadcast( FBCOND *cond )
{
	int previous = fb_DosThreadEnter();
	if( cond ) {
		while( cond->head )
			fb_CondSignal( cond );
	}
	fb_DosThreadLeave( previous );
}

FBCALL void fb_CondWait( FBCOND *cond, FBMUTEX *mutex )
{
	FB_DOS_WAITER waiter = { NULL, 0 };
	int previous;
	if( !cond || !mutex || !fb_DosThreadInit() )
		return;
	previous = fb_DosThreadEnter();
	/* A recursive mutex must have exactly one acquisition at a wait. */
	assert( mutex->owner == lwp_getpid() && mutex->depth == 1 );
	if( cond->tail )
		cond->tail->next = &waiter;
	else
		cond->head = &waiter;
	cond->tail = &waiter;
	/* Release through the mutex queue while scheduling remains excluded. */
	fb_MutexUnlock( mutex );
	_lwp_cur->waiting.what_int = &waiter.signaled;
	_lwp_cur->status = LWP_WAIT_TRUE;
	fb_DosThreadLeave( previous );
	lwp_yield();
	fb_MutexLock( mutex );
}

#else

FBCALL FBCOND *fb_CondCreate( void )
{
	return NULL;
}

FBCALL void fb_CondDestroy( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondSignal( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondBroadcast( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondWait( FBCOND *cond, FBMUTEX *mutex )
{
	(void)cond;
	(void)mutex;
}

#endif

/* end of thread_cond.c */
