/* FreeBASIC DOS runtime: thread_mutex.c
 * Recursive mutex ownership for the optional preemptive scheduler. Scheduler
 * exclusion protects ownership changes. Queued handoff prevents a busy owner
 * from reacquiring on every scheduling tick and starving a waiting thread.
 * No hardware interrupt is masked while a caller waits.
 */

#include "../fb.h"
#include "../fb_private_thread.h"

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

typedef struct FB_DOS_MUTEX_WAITER {
	struct FB_DOS_MUTEX_WAITER *next;
	int owner;
	volatile int granted;
} FB_DOS_MUTEX_WAITER;

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	return calloc( 1, sizeof( FBMUTEX ) );
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
	/* As on other targets, destroying a mutex with waiters is a caller error. */
	if( mutex && mutex->depth == 0 && !mutex->head )
		free( mutex );
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
	int previous, self;
	FB_DOS_MUTEX_WAITER waiter = { NULL, 0, 0 };
	if( !mutex )
		return;
	previous = fb_DosThreadEnter();
	self = _lwp_cur ? lwp_getpid() : LWP_MAIN;
	if( mutex->depth == 0 || mutex->owner == self ) {
		assert( mutex->depth != UINT_MAX );
		mutex->owner = self;
		mutex->depth++;
		fb_DosThreadLeave( previous );
		return;
	}
	/* Waiter storage remains on the owning thread's locked stack. Publish
	 * the queue entry and wait state together, before allowing a switch.
	 */
	assert( _lwp_cur != NULL );
	waiter.owner = self;
	if( mutex->tail )
		mutex->tail->next = &waiter;
	else
		mutex->head = &waiter;
	mutex->tail = &waiter;
	_lwp_cur->waiting.what_int = &waiter.granted;
	_lwp_cur->status = LWP_WAIT_TRUE;
	fb_DosThreadLeave( previous );
	lwp_yield();
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
	int previous, self;
	if( !mutex )
		return;
	previous = fb_DosThreadEnter();
	self = _lwp_cur ? lwp_getpid() : LWP_MAIN;
	if( mutex->depth && mutex->owner == self && --mutex->depth == 0 && mutex->head ) {
		FB_DOS_MUTEX_WAITER *waiter = mutex->head;
		mutex->head = waiter->next;
		if( !mutex->head )
			mutex->tail = NULL;
		/* Reserve ownership before waking the waiter. The releasing thread
		 * cannot acquire ahead of it, even if another tick is delayed.
		 */
		mutex->owner = waiter->owner;
		mutex->depth = 1;
		waiter->granted = 1;
	}
	fb_DosThreadLeave( previous );
}

#else

typedef struct FB_DOS_NOOP_MUTEX {
	unsigned char unused;
} FB_DOS_NOOP_MUTEX;

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	/*
		gfxlib2 owns an event mutex for every screen context, including the
		null driver.  A non-threaded DOS build cannot contend on that mutex,
		but it must still supply a handle so screen setup has the same success
		contract as every other runtime configuration.
	*/
	return (FBMUTEX *)calloc( 1, sizeof( FB_DOS_NOOP_MUTEX ) );
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
	free( mutex );
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
	(void)mutex;
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
	(void)mutex;
}

#endif

/* end of thread_mutex.c */
