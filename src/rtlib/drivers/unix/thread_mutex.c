/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/thread_mutex.c
 * Purpose: mutex handling routines.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	FBMUTEX *mutex = (FBMUTEX *)malloc( sizeof( FBMUTEX ) );
	if( !mutex )
		return NULL;

	pthread_mutex_init( &mutex->id, NULL );

	return mutex;
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
	if( mutex ) {
		pthread_mutex_destroy( &mutex->id );
		free( (void *)mutex );
	}
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
	if( mutex ) {
		pthread_mutex_lock( &mutex->id );
	}
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
	if( mutex ) {
		pthread_mutex_unlock( &mutex->id );
	}
}

/* end of drivers/unix/thread_mutex.c */
