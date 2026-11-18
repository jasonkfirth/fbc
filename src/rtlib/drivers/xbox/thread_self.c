/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/thread_self.c
 * Purpose: Provide thread self runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

#ifdef ENABLE_MT
static FBMUTEX *g_threadFlagMutex = NULL;
#endif


FBCALL FBTHREAD *fb_ThreadSelf( void )
{
	FB_FBTHREADCTX *ctx = FB_TLSGETCTX( FBTHREAD );

#ifdef HOST_XBOX
	/*
		nxdk routes the Win32-style TLS calls through its FLS layer.
		Under xemu, child thread cleanup can leave the main thread with
		an allocated but empty FBTHREAD context.  Rebind the current
		thread lazily so ThreadSelf() keeps the normal rtlib contract.
		Created FreeBASIC threads set ctx->self before running user code,
		so this only repairs the executable startup thread.
	*/
	if( ctx->self == NULL ) {
		fb_AllocateMainFBThread();
		ctx = FB_TLSGETCTX( FBTHREAD );
	}
#endif

	return ctx->self;
}

void fb_AllocateMainFBThread( void )
{
	/* As TlsGetCtx will allocate a TLS structure and 
	   space to hold the context, and as the main thread doesn't have
           its own FBTHREAD* - to avoid making two allocations, 
           we tell it the context is big enough to cover both
           and make our own FBTHREAD
	*/
	FB_FBTHREADCTX* ctx = fb_TlsGetCtx(
		FB_TLSKEY_FBTHREAD, 
		sizeof( *ctx ) + sizeof( *( ctx->self ) ),
		fb_FBTHREADCTX_Destructor
	);
	ctx->self = ( FBTHREAD * )( ctx + 1 );
	memset( ctx->self, 0, sizeof( *( ctx->self ) ) );
	ctx->self->flags = FBTHREAD_MAIN;

#ifdef ENABLE_MT
	g_threadFlagMutex = fb_MutexCreate( );
#endif
}

#ifdef ENABLE_MT
void fb_CloseAtomicFBThreadFlagMutex( void )
{
	fb_MutexDestroy( g_threadFlagMutex );
}
#endif

/* 
** even if this is not the multithreaded version of the rtlib
** define fbAtomicSetThreadFlags() anyway as it will be called
** from thread_core.c:threadproc()
*/
FBTHREADFLAGS fb_AtomicSetThreadFlags( volatile FBTHREADFLAGS *threadFlags, FBTHREADFLAGS newFlag )
{
#ifdef ENABLE_MT
	FBTHREADFLAGS before;
	fb_MutexLock( g_threadFlagMutex );
	before = *threadFlags;
	*threadFlags |= newFlag;
	fb_MutexUnlock( g_threadFlagMutex );
	return before;

#else /* !define(ENABLE_MT) */
	FBTHREADFLAGS before;
	before = *threadFlags;
	*threadFlags |= newFlag;
	return before;

#endif /* ENABLE_MT */
}

/* end of drivers/xbox/thread_self.c */
