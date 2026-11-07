/*
 * Project: FreeBASIC GOSUB runtime qualification
 * File: c-gosub-runtime-targets.c
 * Purpose: Check opaque target-stack ownership and legacy compatibility.
 * Responsibilities: Count node allocations, inject one allocation failure and
 *     verify LIFO order, ERR transitions and mixed-protocol rejection.
 * This file intentionally does NOT inspect private node layouts.
 */
#include "fb.h"
#include "fb_gosub.h"
#include <limits.h>
#include <setjmp.h>

void *gosub_test_malloc( size_t bytes );
void gosub_test_free( void *pointer );

static unsigned int allocations;
static unsigned int releases;
static unsigned int live_nodes;
static unsigned int peak_nodes;
static int refuse_next;
static void *legacy_context;

static void require( int condition, int line )
{
	if( !condition ) {
		fprintf( stderr, "GOSUB runtime assertion at %d\n", line );
		exit( 1 );
	}
}

#define REQUIRE(condition) require( (condition), __LINE__ )

void *gosub_test_malloc( size_t bytes )
{
	void *pointer;
	if( refuse_next ) {
		refuse_next = 0;
		return NULL;
	}
	pointer = malloc( bytes );
	if( pointer != NULL ) {
		allocations++;
		live_nodes++;
		if( live_nodes > peak_nodes )
			peak_nodes = live_nodes;
	}
	return pointer;
}

void gosub_test_free( void *pointer )
{
	if( pointer != NULL ) {
		REQUIRE( live_nodes > 0 );
		live_nodes--;
		releases++;
	}
	free( pointer );
}

int main( int argc, char **argv )
{
	void *context = NULL;
	void *before;
	void *buffer;
	unsigned int index;
	volatile int resumed = 0;
	fb_Init( argc, argv, 0 );
	fb_ErrorSetNum( 37 );
	for( index = 1; index <= 513; index++ ) {
		REQUIRE( fb_GosubPushTarget( &context, index ) == FB_RTERROR_OK );
		REQUIRE( fb_ErrorGetNum() == 37 );
	}
	for( index = 513; index > 0; index-- ) {
		REQUIRE( fb_GosubReturnTarget( &context ) == index );
		REQUIRE( fb_ErrorGetNum() == 37 );
	}
	REQUIRE( context == NULL && live_nodes == 0 );
	REQUIRE( fb_GosubPushTarget( &context, UINT_MAX ) == FB_RTERROR_OK );
	REQUIRE( fb_GosubReturnTarget( &context ) == UINT_MAX );
	REQUIRE( fb_GosubReturnTarget( &context ) == 0 );
	REQUIRE( fb_ErrorGetNum() == FB_RTERROR_RETURNWITHOUTGOSUB );
	REQUIRE( fb_GosubPushTarget( NULL, 1 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	REQUIRE( fb_GosubPushTarget( &context, 0 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	REQUIRE( context == NULL );
	REQUIRE( fb_GosubPushTarget( &context, 7 ) == FB_RTERROR_OK );
	before = context;
	refuse_next = 1;
	REQUIRE( fb_GosubPushTarget( &context, 8 ) == FB_RTERROR_OUTOFMEM );
	REQUIRE( context == before && live_nodes == 1 );
	REQUIRE( fb_GosubReturn( &context ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	REQUIRE( context == before );
	REQUIRE( fb_GosubPush( &context ) != NULL );
	before = context;
	REQUIRE( fb_GosubReturnTarget( &context ) == 0 );
	REQUIRE( fb_ErrorGetNum() == FB_RTERROR_ILLEGALFUNCTIONCALL && context == before );
	REQUIRE( fb_GosubPop( &context ) == FB_RTERROR_OK );
	REQUIRE( fb_GosubReturnTarget( &context ) == 7 );
	REQUIRE( context == NULL && live_nodes == 0 );
	for( index = 1; index <= 19; index++ )
		REQUIRE( fb_GosubPushTarget( &context, index ) == FB_RTERROR_OK );
	fb_ErrorSetNum( 39 );
	fb_GosubExit( &context );
	REQUIRE( context == NULL && live_nodes == 0 && fb_ErrorGetNum() == 39 );
	/* A static context is used for the legacy longjmp control. An automatic
	 * context modified after setjmp would itself have an indeterminate value. */
	buffer = fb_GosubPush( &legacy_context );
	REQUIRE( buffer != NULL );
	if( setjmp( *(jmp_buf *)buffer ) == 0 ) {
		fb_ErrorSetNum( 41 );
		fb_GosubReturn( &legacy_context );
		REQUIRE( 0 );
	}
	resumed = 1;
	REQUIRE( resumed == 1 && legacy_context == NULL && fb_ErrorGetNum() == 41 );
	REQUIRE( live_nodes == 0 && allocations == releases && peak_nodes == 513 );
	puts( "GOSUB_RUNTIME_TARGETS_PASS" );
	fb_End( 0 );
	return 0;
}

/* end of c-gosub-runtime-targets.c */
