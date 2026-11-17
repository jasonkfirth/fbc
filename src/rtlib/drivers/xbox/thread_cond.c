/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/thread_cond.c
 * Purpose: condition variable functions.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

typedef struct FB_COND_WAITER_ {
	KEVENT event;
	struct FB_COND_WAITER_ *next;
} FB_COND_WAITER;

struct FBCOND_ {
	CRITICAL_SECTION lock;
	FB_COND_WAITER *first;
	FB_COND_WAITER *last;
};

FBCALL FBCOND *fb_CondCreate( void )
{
	FBCOND *cond = malloc( sizeof(*cond) );
	if( cond == NULL )
		return NULL;
	InitializeCriticalSection( &cond->lock );
	cond->first = NULL;
	cond->last = NULL;
	return cond;
}

FBCALL void fb_CondDestroy( FBCOND *cond )
{
	/* Like the other providers, destruction requires that callers have
	   joined all waiters. Each waiter owns its event on its own stack. */
	if( cond != NULL ) {
		DeleteCriticalSection( &cond->lock );
		free( cond );
	}
}

FBCALL void fb_CondSignal( FBCOND *cond )
{
	FB_COND_WAITER *waiter;
	if( cond == NULL )
		return;
	EnterCriticalSection( &cond->lock );
	waiter = cond->first;
	if( waiter != NULL ) {
		cond->first = waiter->next;
		if( cond->first == NULL )
			cond->last = NULL;
		KeSetEvent( &waiter->event, 0, FALSE );
	}
	LeaveCriticalSection( &cond->lock );
}

FBCALL void fb_CondBroadcast( FBCOND *cond )
{
	FB_COND_WAITER *waiter, *next;
	if( cond == NULL )
		return;
	EnterCriticalSection( &cond->lock );
	waiter = cond->first;
	cond->first = NULL;
	cond->last = NULL;
	while( waiter != NULL ) {
		next = waiter->next;
		KeSetEvent( &waiter->event, 0, FALSE );
		waiter = next;
	}
	LeaveCriticalSection( &cond->lock );
}

FBCALL void fb_CondWait( FBCOND *cond, FBMUTEX *mutex )
{
	FB_COND_WAITER waiter;
	if( cond == NULL || mutex == NULL )
		return;

	/* Register before releasing the caller's mutex. The installed nxdk
	   condition helper increments its waiter count after releasing that
	   mutex, which lets a concurrent broadcast miss a new waiter.
	   A private synchronization event retains an early signal and does not
	   require a kernel handle allocation for every wait. */
	KeInitializeEvent( &waiter.event, SynchronizationEvent, FALSE );
	waiter.next = NULL;
	EnterCriticalSection( &cond->lock );
	if( cond->last != NULL )
		cond->last->next = &waiter;
	else
		cond->first = &waiter;
	cond->last = &waiter;
	LeaveCriticalSection( &cond->lock );

	LeaveCriticalSection( &mutex->id );
	KeWaitForSingleObject( &waiter.event, Executive, KernelMode, FALSE, NULL );
	/* A signaler still owns the queue lock while touching this stack event.
	   Wait for it before the waiter and its event leave this function. */
	EnterCriticalSection( &cond->lock );
	LeaveCriticalSection( &cond->lock );
	EnterCriticalSection( &mutex->id );
}

/* end of drivers/xbox/thread_cond.c */
