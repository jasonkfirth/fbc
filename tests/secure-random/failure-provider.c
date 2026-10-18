/*
 * FreeBASIC runtime tests: failure-provider.c
 * Supply controlled partial failures to the public secure-random boundary.
 * The single test owner selects a mode before each call. This file is linked
 * only into the failure test, never into the runtime or live-provider test.
 */

#include "fb.h"
#include "fb_secure_random.h"

static int selected_mode;

void TestSecureRandomMode( int mode )
{
	selected_mode = mode;
}

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	size_t changed = byte_count < 7 ? byte_count : 7;
	memset( destination, 0x6b, changed );
	if( selected_mode == 1 )
		return FB_SECURE_RANDOM_UNAVAILABLE;
	if( selected_mode == 2 )
		return FB_SECURE_RANDOM_IO_ERROR;
	if( selected_mode == 3 )
		return 99; /* An invalid provider result must never become success. */
	memset( destination, 0x6b, byte_count );
	return FB_SECURE_RANDOM_OK;
}

/* end of failure-provider.c */
