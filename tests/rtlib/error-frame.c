/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/error-frame.c
 * Purpose: Verify the production C error-frame bridge and legacy compatibility.
 * Responsibilities: Check owner/worker storage, nested error state, cleanup,
 *     opaque legacy save/restore and unchanged error/TLS layouts.
 * This file intentionally does NOT stand in for BASIC handler outlining tests.
 */
#include "fb.h"
#include <stddef.h>
#include <setjmp.h>

typedef struct {
	FB_ERRHANDLER handler;
	int err_num;
	int line_num;
	const char *mod_name;
	const char *fun_name;
	void *res_lbl;
	void *resnxt_lbl;
} LEGACY_ERRORCTX;

typedef char CHECK_ERROR_SIZE[(sizeof( FB_ERRORCTX ) == sizeof( LEGACY_ERRORCTX )) ? 1 : -1];
typedef char CHECK_ERROR_OFFSETS[(offsetof( FB_ERRORCTX, resnxt_lbl ) == offsetof( LEGACY_ERRORCTX, resnxt_lbl )) ? 1 : -1];
/* Existing TLS key values are consumed by precompiled runtime objects. */
typedef char CHECK_TLS_KEYS[(FB_TLSKEY_ERROR == 0 && FB_TLSKEY_DIR == 1 && FB_TLSKEY_INPUT == 2 &&
                             FB_TLSKEY_PRINTUSG == 3 && FB_TLSKEY_GFX == 4 && FB_TLSKEY_FBTHREAD == 5 &&
                             FB_TLSKEY_PROFILE == 6 && FB_TLSKEY_USERVAR == 7 && FB_TLSKEY_ERRORFRAME == 8) ? 1 : -1];

enum {
	MODE_NEXT,
	MODE_RETRY,
	MODE_TRANSFER,
	MODE_NESTED,
	MODE_LEGACY_LOCAL,
	MODE_NESTED_OWNER,
	MODE_UNHANDLED,
	MODE_REENTER,
	MODE_NO_RESUME,
	MODE_TRANSFER_HEAP
};

typedef struct OWNER OWNER;
typedef struct WORKER WORKER;

struct OWNER {
	FB_ERROR_FRAME frame;
	void *buffer;
	WORKER *current;
	int guard;
	int callbacks;
	int cleaned;
	int worker_reached;
	int mode;
	int failed;
};

struct WORKER {
	FB_ERROR_FRAME frame;
	OWNER *owner;
	void *resource;
	int guard;
	int attempts;
};

static int run_case( int mode );

static FB_ERRHANDLER legacy_address( void *address )
{
	FB_ERRHANDLER result;
	memcpy( &result, &address, sizeof( result ) );
	return result;
}

static void *data_address( FB_ERRHANDLER address )
{
	void *result;
	memcpy( &result, &address, sizeof( result ) );
	return result;
}

static void cleanup_worker( void *opaque )
{
	WORKER *worker = (WORKER *)opaque;
	FB_ERROR_FRAME nested;
	if( fb_ErrorFrameEnter( &nested, NULL, NULL ) != FB_RTERROR_OK ||
	    fb_ErrorFrameLeave( &nested ) != FB_RTERROR_OK )
		worker->owner->failed = 23;
	if( worker->guard != 303 )
		worker->owner->failed = 1;
	free( worker->resource );
	worker->resource = NULL;
	worker->owner->cleaned++;
}

static void cleanup_heap_worker( void *opaque )
{
	cleanup_worker( opaque );
	free( opaque );
}

static int nested_handler( void *opaque )
{
	OWNER *owner = (OWNER *)opaque;
	if( fb_ErrorGetNum() != 25 || fb_ErrorGetLineNum() != 200 )
		owner->failed = 2;
	return FB_ERROR_FRAME_NEXT;
}

static int owner_handler( void *opaque )
{
	OWNER *owner = (OWNER *)opaque;
	int current_marker = 0, next_marker = 0, legacy_marker = 0;
	void *previous = NULL;
	FB_ERRHANDLER saved;
	FB_ERRHANDLER target;

	if( fb_ErrorGetNum() != FB_RTERROR_FILEIO || fb_ErrorGetLineNum() != 100 ||
	    owner->guard != 103 + owner->callbacks || owner->current == NULL || owner->current->guard != 303 )
		owner->failed = 3;
	if( fb_ErrorFrameLeave( &owner->frame ) != FB_RTERROR_ILLEGALFUNCTIONCALL ||
	    fb_ErrorFrameLeave( &owner->current->frame ) != FB_RTERROR_ILLEGALFUNCTIONCALL )
		owner->failed = 4;
	owner->guard++;
	owner->callbacks++;
	if( owner->mode == MODE_UNHANDLED )
		return FB_ERROR_FRAME_UNHANDLED;
	if( owner->mode == MODE_REENTER )
		fb_ErrorThrowEx( FB_RTERROR_FILEIO, 200, "reentry", &current_marker, &next_marker );
	if( owner->mode == MODE_TRANSFER || owner->mode == MODE_TRANSFER_HEAP ) {
		if( fb_ErrorFrameSetTarget( &owner->frame, 1 ) != FB_RTERROR_OK )
			owner->failed = 5;
		return FB_ERROR_FRAME_TRANSFER;
	}
	if( owner->mode == MODE_NESTED ) {
		if( fb_ErrorFrameSetHandler( &owner->frame, nested_handler, &previous ) != FB_RTERROR_OK )
			owner->failed = 6;
		target = fb_ErrorThrowEx( 25, 200, "nested", &current_marker, &next_marker );
		if( data_address( target ) != &next_marker || fb_ErrorGetNum() != FB_RTERROR_FILEIO || fb_ErrorGetLineNum() != 100 )
			owner->failed = 7;
		fb_ErrorFrameRestore( previous );
	}
	if( owner->mode == MODE_LEGACY_LOCAL ) {
		saved = fb_ErrorSetHandler( legacy_address( &legacy_marker ) );
		target = fb_ErrorThrowEx( 25, 200, "legacy-local", &current_marker, &next_marker );
		if( data_address( target ) != &legacy_marker || fb_ErrorResumeNext() != &next_marker ||
		    fb_ErrorGetNum() != FB_RTERROR_FILEIO || fb_ErrorGetLineNum() != 100 )
			owner->failed = 8;
		fb_ErrorSetHandler( saved );
	}
	if( owner->mode == MODE_NESTED_OWNER ) {
		if( run_case( MODE_TRANSFER ) != 0 || fb_ErrorGetNum() != FB_RTERROR_FILEIO || fb_ErrorGetLineNum() != 100 )
			owner->failed = 9;
	}
	if( owner->mode == MODE_RETRY && owner->callbacks == 1 )
		return FB_ERROR_FRAME_RETRY;
	return FB_ERROR_FRAME_NEXT;
}

static int worker_call( OWNER *owner )
{
	WORKER automatic_worker;
	WORKER *heap_worker = NULL;
	WORKER *worker = &automatic_worker;
	FB_ERROR_FRAME_CLEANUP cleanup = cleanup_worker;
	int current_marker = 0, next_marker = 0;
	FB_ERRHANDLER target;
	int status;

	if( owner->mode == MODE_TRANSFER_HEAP ) {
		heap_worker = (WORKER *)calloc( 1, sizeof( *heap_worker ) );
		if( heap_worker == NULL )
			return 24;
		worker = heap_worker;
		cleanup = cleanup_heap_worker;
	}
	worker->owner = owner;
	worker->resource = malloc( 1 );
	worker->guard = 303;
	worker->attempts = 0;
	if( worker->resource == NULL ) {
		free( heap_worker );
		return 10;
	}
	status = fb_ErrorFrameEnter( &worker->frame, worker, cleanup );
	if( status != FB_RTERROR_OK ) {
		free( worker->resource );
		free( heap_worker );
		return 11;
	}
	owner->current = worker;
retry:
	worker->attempts++;
	target = fb_ErrorThrowEx( FB_RTERROR_FILEIO, 100, "worker",
	                         owner->mode == MODE_NO_RESUME ? NULL : &current_marker,
	                         owner->mode == MODE_NO_RESUME ? NULL : &next_marker );
	if( data_address( target ) == &current_marker )
		goto retry;
	if( data_address( target ) != &next_marker || worker->guard != 303 ||
	    worker->attempts != (owner->mode == MODE_RETRY ? 2 : 1) )
		owner->failed = 12;
	owner->worker_reached++;
	owner->current = NULL;
	status = fb_ErrorFrameLeave( &worker->frame );
	cleanup( worker );
	return status;
}

static int run_case( int mode )
{
	OWNER *owner;
	void *previous = NULL;
	int result;

	owner = (OWNER *)calloc( 1, sizeof( *owner ) );
	if( owner == NULL )
		return 13;
	owner->guard = 103;
	owner->mode = mode;
	if( fb_ErrorFrameEnter( &owner->frame, owner, NULL ) != FB_RTERROR_OK ) {
		free( owner );
		return 14;
	}
	owner->buffer = fb_ErrorFrameBuffer( &owner->frame );
	if( owner->buffer == NULL ) {
		fb_ErrorFrameLeave( &owner->frame );
		free( owner );
		return 15;
	}
	if( setjmp( *(jmp_buf *)owner->buffer ) == 0 ) {
		if( fb_ErrorFrameReady( &owner->frame ) != FB_RTERROR_OK ||
		    fb_ErrorFrameSetHandler( &owner->frame, owner_handler, &previous ) != FB_RTERROR_OK )
			owner->failed = 16;
		if( worker_call( owner ) != FB_RTERROR_OK )
			owner->failed = 17;
	} else {
		if( (mode != MODE_TRANSFER && mode != MODE_TRANSFER_HEAP) || fb_ErrorFrameGetTarget( &owner->frame ) != 1 ||
		    fb_ErrorGetNum() != FB_RTERROR_FILEIO )
			owner->failed = 18;
		owner->current = NULL;
	}
	if( owner->cleaned != 1 || owner->worker_reached != ((mode == MODE_TRANSFER || mode == MODE_TRANSFER_HEAP) ? 0 : 1) ||
	    owner->callbacks != (mode == MODE_RETRY ? 2 : 1) || owner->guard != 103 + owner->callbacks )
		owner->failed = 19;
	if( fb_ErrorFrameLeave( &owner->frame ) != FB_RTERROR_OK )
		owner->failed = 20;
	result = owner->failed;
	free( owner );
	return result;
}

static int legacy_case( void )
{
	int handler = 0, current_marker = 0, next_marker = 0;
	FB_ERRHANDLER old = fb_ErrorSetHandler( legacy_address( &handler ) );
	FB_ERRHANDLER target = fb_ErrorThrowEx( FB_RTERROR_FILEIO, 12, "legacy", &current_marker, &next_marker );
	int failed = data_address( target ) != &handler || fb_ErrorGetNum() != FB_RTERROR_FILEIO;
	if( fb_ErrorResumeNext() != &next_marker || fb_ErrorGetNum() != 0 )
		failed = 1;
	fb_ErrorSetHandler( old );
	return failed;
}

#ifdef TEST_ALLOCATOR
static int allocation_fail_after = -1;
static int allocation_calls;
void *fb_error_frame_test_calloc( size_t count, size_t bytes );

void *fb_error_frame_test_calloc( size_t count, size_t bytes )
{
	allocation_calls++;
	if( allocation_fail_after == 0 )
		return NULL;
	if( allocation_fail_after > 0 )
		allocation_fail_after--;
	return calloc( count, bytes );
}

static int allocation_cases( void )
{
	FB_ERROR_FRAME frame;
	void *buffer;
	int marker = 0;
	void *previous = &marker;
	int calls, index;

	if( fb_ErrorFrameEnter( &frame, NULL, NULL ) != FB_RTERROR_OK )
		return 25;
	allocation_fail_after = 0;
	buffer = fb_ErrorFrameBuffer( &frame );
	allocation_fail_after = -1;
	if( buffer != NULL || fb_ErrorGetNum() != FB_RTERROR_OUTOFMEM || fb_ErrorFrameLeave( &frame ) != FB_RTERROR_OK )
		return 26;
	if( fb_ErrorFrameEnter( &frame, NULL, NULL ) != FB_RTERROR_OK )
		return 27;
	allocation_fail_after = 1;
	buffer = fb_ErrorFrameBuffer( &frame );
	if( buffer == NULL || fb_ErrorFrameSetHandler( &frame, owner_handler, &previous ) != FB_RTERROR_OUTOFMEM || previous != &marker )
		return 28;
	allocation_fail_after = -1;
	if( fb_ErrorFrameSetHandler( &frame, owner_handler, &previous ) != FB_RTERROR_OK )
		return 29;
	calls = allocation_calls;
	allocation_fail_after = 0;
	for( index = 0; index < 512; index++ ) {
		if( fb_ErrorFrameSetHandler( &frame, owner_handler, &previous ) != FB_RTERROR_OK )
			return 30;
	}
	allocation_fail_after = -1;
	if( allocation_calls != calls || fb_ErrorFrameLeave( &frame ) != FB_RTERROR_OK )
		return 31;
	fb_ErrorSetNum( 0 );
	return 0;
}
#endif

#ifdef TEST_MT
typedef struct {
	int result;
} THREAD_RESULT;

static void FBCALL thread_case( void *opaque )
{
	THREAD_RESULT *result = (THREAD_RESULT *)opaque;
	result->result = run_case( MODE_NESTED_OWNER );
}

static int thread_cases( void )
{
	THREAD_RESULT left = {1}, right = {1};
	FBTHREAD *left_thread;
	FBTHREAD *right_thread;
	int previous = fb_ErrorGetNum();

	left_thread = fb_ThreadCreate( thread_case, &left, 0 );
	right_thread = fb_ThreadCreate( thread_case, &right, 0 );
	if( left_thread != NULL ) fb_ThreadWait( left_thread );
	if( right_thread != NULL ) fb_ThreadWait( right_thread );
	if( left.result != 0 || right.result != 0 || fb_ErrorGetNum() != previous )
		return 32;
	return 0;
}
#endif

int main( void )
{
	const char *mode = getenv( "FBC_ERROR_FRAME_TEST_MODE" );
	int result;

	fb_Init( 0, NULL, 0 );
	/* The unchanged legacy object allocates ERROR first, using its old size. */
	if( fb_ErrorGetNum() != 0 || legacy_case() != 0 )
		return 21;
	/* Legacy throw/resume must not allocate optional callback state. */
	if( fb_TlsGetCtxExisting( FB_TLSKEY_ERRORFRAME ) != NULL ||
	    fb_TlsGetCtxExisting( -1 ) != NULL || fb_TlsGetCtxExisting( FB_TLSKEYS ) != NULL )
		return 33;
	if( mode != NULL && *mode == 'u' )
		return run_case( MODE_UNHANDLED );
	if( mode != NULL && *mode == 'r' )
		return run_case( MODE_REENTER );
	if( mode != NULL && *mode == 'n' )
		return run_case( MODE_NO_RESUME );
#ifdef TEST_ALLOCATOR
	result = allocation_cases();
	if( result != 0 )
		return result;
#endif
	result = run_case( MODE_NEXT );
	if( result == 0 ) result = run_case( MODE_RETRY );
	if( result == 0 ) result = run_case( MODE_TRANSFER );
	if( result == 0 ) result = run_case( MODE_NESTED );
	if( result == 0 ) result = run_case( MODE_LEGACY_LOCAL );
	if( result == 0 ) result = run_case( MODE_NESTED_OWNER );
	if( result == 0 ) result = run_case( MODE_TRANSFER_HEAP );
#ifdef TEST_MT
	if( result == 0 ) result = thread_cases();
#endif
	if( result != 0 ) {
		printf( "error-frame-runtime-failed %d\n", result );
		return result;
	}
	fb_TlsFreeCtxTb();
	if( puts( "error-frame-runtime-ok" ) < 0 )
		return 22;
	return 0;
}

/* end of rtlib/error-frame.c */
