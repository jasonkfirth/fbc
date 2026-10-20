/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/thread_mutex.c
 * Purpose: mutex handling routines.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	FBMUTEX *mutex = (FBMUTEX *)malloc( sizeof( FBMUTEX ) );
	if( mutex == NULL ) {
		return NULL;
	}

	InitializeCriticalSection( &mutex->id );

	return mutex;
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
	if( mutex != NULL ) {
		DeleteCriticalSection( &mutex->id );
		free( (void *)mutex );
	}
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
	if( mutex != NULL ) {
		EnterCriticalSection( &mutex->id );
	}
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
	if( mutex != NULL ) {
		LeaveCriticalSection( &mutex->id );
	}
}

/* end of drivers/xbox/thread_mutex.c */
