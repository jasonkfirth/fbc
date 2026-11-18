/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/init.c
 * Purpose: libfb initialization.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_thread.h"

FB_RTLIB_CTX fb_ctx /* not initialized */;
static int fb_runtime_init_count = 0;

/* called from fbrt0 */
void fb_hRtInit( void )
{
	/* already initialized? */
	++fb_runtime_init_count;
	if( fb_runtime_init_count != 1 )
		return;

	/* initialize context */
	memset( &fb_ctx, 0, sizeof( FB_RTLIB_CTX ) );
    
	/* os-dep initialization */
	fb_hInit( );

#ifdef ENABLE_MT
	fb_TlsInit( );
#endif
	fb_AllocateMainFBThread();

	/* Register once, before BASIC global constructors. The fb_Init fallback
	   also reaches this path when a caller deliberately omits fbrt0. */
	if( atexit( fb_hRtExit ) != 0 ) {
		fb_hRtExit();
		abort();
	}
}

/* called from fbrt0 */
void fb_hRtExit( void )
{
	--fb_runtime_init_count;
	if( fb_runtime_init_count != 0 )
		return;

	/* Doing clean-up here in the rtlib's global dtor, instead of using
	   atexit().

	   FB supports global ctors/dtors, and thus FB programs can call FB
	   functions from there. Hence the rtlib/gfxlib2 initialization should
	   be the first global ctor, and the clean-up should be the last global
	   dtor. This is done by fbrt0.c. We can't rely on atexit() for this,
	   because sometimes the atexit handlers can be called before the global
	   dtors, sometimes after.

	   However, on Windows it's not safe to do certain things such as
	   waiting for a thread to finish from inside a global
	   constructor/destructor due to the loader lock. (A lock is acquired
	   to synchronize calls to DllMain() etc., so waiting for a thread from
	   inside a global destructor may cause a dead-lock, as that thread
	   needs to acquire the same lock to be able to exit.) Because of this,
	   gfxlib2 clean-up can't be done during a global dtor, at least not on
	   Windows, because its win32 backends use a background thread. So the
	   gfxlib clean-up must be done during fb_End() instead.

	   Some observations about atexit() behaviour: it depends on context,
	   e.g. whether it's called from a global ctor or global dtor or main(),
	   or whether it's a shared lib/DLL or executable,
	   and it depends on the platform (e.g. GNU/Linux vs MinGW-w64).
	   Thus it can't be used reliably. */

	fb_FileReset( );

	/* os-dep termination */
	fb_hEnd( 0 );

	fb_DevScrnEnd( FB_HANDLE_SCREEN );

	/* Free main thread's TLS contexts */
	fb_TlsFreeCtxTb( );

#ifdef ENABLE_MT
	fb_TlsExit( );
#endif

	/* If an error message was stored, print it now, after the console was
	   cleaned up. At least the DOS gfxlib clears the console on exit,
	   thus any error messages must be printed after that or they would
	   not be visible. */
	if( fb_ctx.errmsg )
		fputs( fb_ctx.errmsg, stderr );
}

/* called by FB program */
FBCALL void fb_Init( int argc, char **argv, int lang )
{
	fb_ctx.argc = argc;
	fb_ctx.argv = argv;
	fb_ctx.lang = lang;

	/*
		The fbrt0 PE initializer normally prepares the runtime before BASIC
		global constructors. Preserve startup for callers which omit fbrt0,
		without taking a second reference to the initialized runtime.

		If that constructor did run already, it may have registered a
		different startup thread as FreeBASIC's main thread.  Bind the
		actual program thread here so ThreadSelf() reports a usable handle
		from user code.
	*/
	if( fb_runtime_init_count == 0 )
		fb_hRtInit();
	fb_AllocateMainFBThread();
}

/* called by FB program,
   or fb_Die() in case of assert() failure or runtime error */
FBCALL void fb_End( int errlevel )
{
	if( fb_ctx.exit_sfxlib )
		fb_ctx.exit_sfxlib( );

	if( fb_ctx.exit_gfxlib2 )
		fb_ctx.exit_gfxlib2( );

	exit( errlevel );
}

/* end of drivers/xbox/init.c */
