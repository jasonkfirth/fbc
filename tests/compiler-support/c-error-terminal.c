/*
 * Project: FreeBASIC C emitter regression tests
 * File: c-error-terminal.c
 * Purpose: Check owner-frame unwinding and an impossible returned target.
 * Responsibilities: Keep owner data alive across longjmp, count evaluation,
 *     and observe the terminal guard without crashing the host process.
 * This file intentionally does NOT qualify automatic BASIC handler outlining.
 */
#include "fb.h"
#include "c-error-terminal-shim.h"
#include <setjmp.h>

void fb_test_terminal_worker( int *visits );
FB_ERRHANDLER __real_fb_ErrorThrowEx( int errnum, int line, const char *module,
                                    void *resume, void *next );
FB_ERRHANDLER __wrap_fb_ErrorThrowEx( int errnum, int line, const char *module,
                                    void *resume, void *next );

typedef struct {
	FB_ERROR_FRAME frame;
	int callbacks;
	int visits;
} OWNER;

/* Each mode runs in a fresh process. These counters observe only that worker's
 * error call; they are not a replacement for the runtime's thread-local state. */
static int inject_return;
static int evaluations;
static OWNER *active_owner;

FB_ERRHANDLER __wrap_fb_ErrorThrowEx( int errnum, int line, const char *module,
                                    void *resume, void *next )
{
	evaluations++;
	if( inject_return )
		return NULL;
	return __real_fb_ErrorThrowEx( errnum, line, module, resume, next );
}

_Noreturn void fb_test_invalid_transfer( void )
{
	/* The substituted guard is reached only after the mocked runtime returns.
	 * An owner-frame transfer must leave the worker before this point. */
	_Exit( inject_return && evaluations == 1 && active_owner != NULL &&
	       active_owner->visits == 1 ? 0 : 91 );
}

static int handler( void *context )
{
	OWNER *owner = context;
	if( fb_ErrorGetNum() != 24 || owner->visits != 1 )
		return FB_ERROR_FRAME_UNHANDLED;
	owner->callbacks++;
	if( fb_ErrorFrameSetTarget( &owner->frame, 7 ) != 0 )
		return FB_ERROR_FRAME_UNHANDLED;
	return FB_ERROR_FRAME_TRANSFER;
}

int main( int argc, char **argv )
{
	OWNER *owner;
	void *previous;
	int result;
	fb_Init( 0, NULL, FB_LANG_FB );
	owner = calloc( 1, sizeof( *owner ) );
	if( owner == NULL ) return 92;
	active_owner = owner;
	inject_return = (argc > 1 && argv[1][0] == 'i');
	if( inject_return ) {
		fb_test_terminal_worker( &owner->visits );
		return 93;
	}
	/* Only the unchanged owner pointer is automatic across setjmp. Mutable
	 * fields live in heap storage until the owner has left the runtime frame. */
	if( fb_ErrorFrameEnter( &owner->frame, owner, NULL ) != 0 ) return 94;
	if( setjmp( *(jmp_buf *)fb_ErrorFrameBuffer( &owner->frame ) ) == 0 ) {
		if( fb_ErrorFrameReady( &owner->frame ) != 0 ) return 95;
		if( fb_ErrorFrameSetHandler( &owner->frame, handler, &previous ) != 0 ) return 96;
		if( previous != NULL ) return 97;
		fb_test_terminal_worker( &owner->visits );
		return 98;
	}
	result = (owner->callbacks == 1 && owner->visits == 1 && evaluations == 1 &&
	          fb_ErrorFrameGetTarget( &owner->frame ) == 7) ? 0 : 99;
	if( fb_ErrorFrameLeave( &owner->frame ) != 0 ) result = 100;
	active_owner = NULL;
	free( owner );
	fb_TlsFreeCtxTb();
	return result;
}

/* end of c-error-terminal.c */
