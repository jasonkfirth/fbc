/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/thread_mutex.c
 * Purpose: mutex handling routines.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	return NULL;
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
}

/* end of drivers/js/thread_mutex.c */
