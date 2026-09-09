/*
    FreeBASIC Runtime Tests
    File: string/vb-helpers-limits.c
    Purpose: Exercise string helper failures without enormous allocations.
    Responsibilities: Inject allocation failure and probe overflow guards.
    This file does not replace the FreeBASIC tests using the real allocator.
*/

#include "../../src/rtlib/fb.h"
#include <assert.h>

/* Only the services used by these entry points are supplied. The allocator
   always fails, so even a one-byte request must take the real error path. */
FB_RTLIB_CTX __fb_ctx;
static int last_error;
static int allocations;
static int releases;

FBCALL int fb_ErrorSetNum( int error )
{
	last_error = error;
	return error;
}

FBCALL FBSTRING *fb_hStrAllocTemp_NoLock( FBSTRING *str, ssize_t len )
{
	(void)str;
	assert( len > 0 );
	++allocations;
	return NULL;
}

FBCALL int fb_hStrDelTemp_NoLock( FBSTRING *str )
{
	if( str )
		++releases;
	return 0;
}

int main( void )
{
	FBSTRING src = { "aa", 2, 2 };
	FBSTRING find = { "a", 1, 1 };
	FBSTRING huge = { "x", FB_STRSIZEMSK, FB_STRSIZEMSK };
	FBSTRING *result;

	result = fb_StrReplace( &src, &find, &huge, 1, -1, 0 );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OUTOFMEM );
	assert( allocations == 0 && releases == 3 );
	result = fb_StrReverse( &huge );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OUTOFMEM && allocations == 0 );

	result = fb_StrReplace( &src, &find, &src, 1, -1, 0 );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OUTOFMEM && allocations == 1 );
	result = fb_StrReverse( &src );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OUTOFMEM && allocations == 2 );

	result = fb_StrReplace( &src, &find, &huge, FB_STRSIZEMSK, -1, 0 );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OK && allocations == 2 );
	result = fb_StrReplace( &src, &find, &src, -FB_STRSIZEMSK - 1, -1, 0 );
	assert( result == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_ILLEGALFUNCTIONCALL );

	releases = 0;
	assert( fb_StrComp( &src, &src, 0 ) == 0 );
	assert( releases == 1 );
	fb_StrReplace( &src, &src, &src, 0, -1, 0 );
	assert( releases == 2 );
	assert( fb_StrComp( NULL, &src, 0 ) == -1 );
	assert( fb_StrComp( NULL, NULL, 0 ) == 0 );
	assert( fb_StrReverse( NULL ) == &__fb_ctx.null_desc );
	assert( fb_StrReplace( NULL, NULL, NULL, 1, -1, 0 ) == &__fb_ctx.null_desc );
	assert( last_error == FB_RTERROR_OK );
	puts( "VB string allocation/overflow checks passed" );
	return 0;
}

/* end of string/vb-helpers-limits.c */
