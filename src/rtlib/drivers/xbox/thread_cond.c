/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/thread_cond.c
 * Purpose: condition variable functions.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL FBCOND *fb_CondCreate( void )
{
	return NULL;
}

FBCALL void fb_CondDestroy( FBCOND *cond )
{
}

FBCALL void fb_CondSignal( FBCOND *cond )
{
}

FBCALL void fb_CondBroadcast( FBCOND *cond )
{
}

FBCALL void fb_CondWait( FBCOND *cond, FBMUTEX *mutex )
{
}

/* end of drivers/xbox/thread_cond.c */
