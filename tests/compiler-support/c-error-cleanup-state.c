/*
 * Project: FreeBASIC runtime regression tests
 * File: c-error-cleanup-state.c
 * Purpose: Preserve caller error state while retiring abandoned C activations.
 * Responsibilities: Check callback order, nested handlers and thread-local state.
 * This file intentionally does NOT invoke native handlers or qualify mixed-backend jumps.
 */
#include "fb.h"
#include <setjmp.h>

#if defined(FB_TEST_PRIVATE_ERROR_RUNTIME)
#include "fb_error_c_resume.h"
#endif

typedef struct TEST_CASE TEST_CASE;
typedef struct {
	FB_ERROR_FRAME frame;
	TEST_CASE *test;
	int ordinal;
} TEST_WORKER;

struct TEST_CASE {
	FB_ERROR_FRAME frame;
	TEST_WORKER *empty_worker;
	FB_ERRORCTX caller;
	void *previous;
	FB_ERRHANDLER cleanup_handler;
	int error_number, line_number;
	int nested, inside_callback, override_number;
	int callbacks, cleanups, failures;
};

static const char module_name[] = "cleanup-state-module";
static const char function_name[] = "cleanup-state-function";
static int run_transfer( int nested, int inside_callback, int override_number );

static int same_state( const FB_ERRORCTX *left, const FB_ERRORCTX *right )
{
	return left->handler == right->handler && left->err_num == right->err_num &&
	       left->line_num == right->line_num && left->mod_name == right->mod_name &&
	       left->fun_name == right->fun_name && left->res_lbl == right->res_lbl &&
	       left->resnxt_lbl == right->resnxt_lbl;
}

static void cleanup( void *data )
{
	TEST_WORKER *worker = (TEST_WORKER *)data;
	TEST_CASE *test = worker->test;
	FB_ERRORCTX *error = FB_TLSGETCTX( ERROR );
	if( error->err_num != test->error_number || error->line_num != test->line_number ||
	    error->mod_name != module_name || error->fun_name != function_name ||
	    error->handler != test->cleanup_handler || worker->ordinal != 2 - test->cleanups )
		++test->failures;
	++test->cleanups;
	/* Ordinary cleanup calls can set successful return status. Other metadata
	   setters model callbacks that report their own source location temporarily. */
	fb_ErrorSetNum( 0 );
	fb_ErrorSetModName( "cleanup-module" );
	fb_ErrorSetFuncName( "cleanup-function" );
	error->line_num = 3;
	free( worker );
}

static void enter_workers( TEST_CASE *test )
{
	int ordinal;
	for( ordinal = 1; ordinal <= 2; ++ordinal ) {
		TEST_WORKER *worker = (TEST_WORKER *)calloc( 1, sizeof( *worker ) );
		if( worker == NULL ) exit( 81 );
		worker->test = test;
		worker->ordinal = ordinal;
		if( fb_ErrorFrameEnter( &worker->frame, worker, cleanup ) != 0 ) exit( 82 );
		if( ordinal == 1 ) {
			/* A null callback must retire its frame without affecting state. */
			test->empty_worker = (TEST_WORKER *)calloc( 1, sizeof( *worker ) );
			if( test->empty_worker == NULL ) exit( 83 );
			if( fb_ErrorFrameEnter( &test->empty_worker->frame, NULL, NULL ) != 0 ) exit( 84 );
		}
	}
}

static int handler( void *data )
{
	TEST_CASE *test = (TEST_CASE *)data;
	FB_ERRORCTX *error = FB_TLSGETCTX( ERROR );
	++test->callbacks;
	if( error->err_num != test->error_number || error->line_num != test->line_number )
		++test->failures;
	if( test->nested ) {
		FB_ERRORCTX before = *error;
		if( run_transfer( 0, 1, 0 ) != 0 || !same_state( &before, error ) )
			++test->failures;
	}
	/* Preserve the post-handler context, including an intentional ERR change. */
	if( test->override_number ) {
		test->error_number = 55;
		fb_ErrorSetNum( test->error_number );
	}
	if( fb_ErrorFrameSetTarget( &test->frame, 7 ) != 0 ) exit( 85 );
	return FB_ERROR_FRAME_TRANSFER;
}

static int run_transfer( int nested, int inside_callback, int override_number )
{
	TEST_CASE *const test = (TEST_CASE *)calloc( 1, sizeof( *test ) );
	int result;
	if( test == NULL ) exit( 86 );
	test->caller = *FB_TLSGETCTX( ERROR );
	test->nested = nested;
	test->inside_callback = inside_callback;
	test->override_number = override_number;
	test->error_number = inside_callback ? 47 : 24;
	test->line_number = inside_callback ? 201 : 101;
	if( fb_ErrorFrameEnter( &test->frame, test, NULL ) != 0 ) exit( 87 );
	/* Only the unchanged pointer crosses setjmp; mutable state lives in the
	   allocated record until its owner checkpoint has left the runtime. */
	if( setjmp( *(jmp_buf *)fb_ErrorFrameBuffer( &test->frame ) ) == 0 ) {
		if( fb_ErrorFrameReady( &test->frame ) != 0 ) exit( 88 );
		if( fb_ErrorFrameSetHandler( &test->frame, handler, &test->previous ) != 0 ) exit( 89 );
		test->cleanup_handler = FB_TLSGETCTX( ERROR )->handler;
		enter_workers( test );
		fb_ErrorThrowEx( test->error_number, test->line_number, module_name, NULL, NULL );
		exit( 90 );
	}
	if( test->callbacks != 1 || test->cleanups != 2 || fb_ErrorFrameGetTarget( &test->frame ) != 7 )
		++test->failures;
	if( inside_callback ) {
		FB_ERRORCTX current = *FB_TLSGETCTX( ERROR );
		current.handler = test->caller.handler;
		if( !same_state( &current, &test->caller ) ) ++test->failures;
	} else {
		FB_ERRORCTX *error = FB_TLSGETCTX( ERROR );
		if( error->err_num != test->error_number || error->line_num != test->line_number ||
		    error->mod_name != module_name || error->fun_name != function_name ||
		    error->res_lbl != NULL || error->resnxt_lbl != NULL )
			++test->failures;
	}
	if( fb_ErrorFrameLeave( &test->frame ) != 0 ) exit( 91 );
	if( FB_TLSGETCTX( ERROR )->handler != test->caller.handler ) ++test->failures;
	result = test->failures != 0;
	free( test->empty_worker );
	free( test );
	return result;
}

#if defined(FB_TEST_PRIVATE_ERROR_RUNTIME)
static void unused_native_handler( void )
{
	exit( 92 );
}

static int run_owner_metadata_cleanup( void )
{
	TEST_CASE *test = (TEST_CASE *)calloc( 1, sizeof( *test ) );
	void *const frame = __builtin_frame_address( 0 );
	FB_ERRORCTX before;
	int result;
	if( test == NULL ) exit( 93 );
	fb_ErrorSetNum( 77 );
	FB_TLSGETCTX( ERROR )->line_num = 301;
	test->caller = *FB_TLSGETCTX( ERROR );
	fb_ErrorSetNativeHandlerSized( unused_native_handler, frame, 32 );
	test->error_number = 77;
	test->line_number = 301;
	test->cleanup_handler = FB_TLSGETCTX( ERROR )->handler;
	before = *FB_TLSGETCTX( ERROR );
	enter_workers( test );
	/* This calls the owner metadata API only. No native handler or jump runs. */
	if( fb_ErrorNativeOwnerCleanup( frame ) != 0 ) exit( 94 );
	if( test->cleanups != 2 || !same_state( &before, FB_TLSGETCTX( ERROR ) ) )
		++test->failures;
	if( fb_ErrorNativeOwnerLeave( frame ) != 0 ) exit( 95 );
	if( FB_TLSGETCTX( ERROR )->handler != test->caller.handler ) ++test->failures;
	result = test->failures != 0;
	free( test->empty_worker );
	free( test );
	return result;
}
#endif

static int run_cases( void )
{
	int iteration, failures = 0;
	for( iteration = 0; iteration < 8; ++iteration ) {
		fb_ErrorSetNum( 0 );
		fb_ErrorSetModName( module_name );
		fb_ErrorSetFuncName( function_name );
		failures += run_transfer( 0, 0, 0 );
		failures += run_transfer( 0, 0, 1 );
		failures += run_transfer( 1, 0, 0 );
#if defined(FB_TEST_PRIVATE_ERROR_RUNTIME)
		failures += run_owner_metadata_cleanup();
#endif
	}
	return failures == 0 ? 0 : 9;
}

#if defined(ENABLE_MT)
static void FBCALL run_thread( void *argument )
{
	int *result = (int *)argument;
	*result = run_cases();
}
#endif

int main( void )
{
	int result = 0;
	fb_Init( 0, NULL, FB_LANG_FB );
#if defined(ENABLE_MT)
	{
		FBTHREAD *threads[2];
		int results[2] = { 9, 9 };
		int index;
		/* Each thread owns its result slot and TLS. ThreadWait joins before
		   the main thread reads the completed result. */
		for( index = 0; index < 2; ++index ) {
			threads[index] = fb_ThreadCreate( run_thread, &results[index], 0 );
			if( threads[index] == NULL ) exit( 96 );
		}
		for( index = 0; index < 2; ++index ) {
			fb_ThreadWait( threads[index] );
			if( results[index] != 0 ) result = 9;
		}
	}
#else
	result = run_cases();
#endif
	fb_TlsFreeCtxTb();
	printf( "CLEANUP_STATE_API_RESULT=%d\n", result );
	return result;
}

/* end of c-error-cleanup-state.c */
