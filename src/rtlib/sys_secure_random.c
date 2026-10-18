/*
 * FreeBASIC runtime: sys_secure_random.c
 * Validate checked system-randomness requests and clear failed output.
 * Platform access belongs to sys_hsecure_random.c. This file contains no
 * entropy algorithm, RND state, allocations or BASIC ERR manipulation.
 */

#include "fb.h"
#include "fb_secure_random.h"

FBCALL int fb_SecureRandom( void *destination, size_t byte_count )
{
	int status;
	volatile unsigned char *clear;
	size_t index;

	if( byte_count > FB_SECURE_RANDOM_MAX_BYTES )
		return FB_SECURE_RANDOM_INVALID_ARGUMENT;
	if( byte_count == 0 )
		return FB_SECURE_RANDOM_OK;
	if( destination == NULL )
		return FB_SECURE_RANDOM_INVALID_ARGUMENT;

	status = fb_hSecureRandom( destination, byte_count );
	if( status == FB_SECURE_RANDOM_OK )
		return FB_SECURE_RANDOM_OK;

	/* A caller must never mistake a partial system read for usable secrets.
	 * Volatile stores retain the wipe even when the caller discards its buffer.
	 */
	clear = destination;
	for( index = 0; index < byte_count; ++index )
		clear[index] = 0;

	if( status == FB_SECURE_RANDOM_UNAVAILABLE )
		return status;
	return FB_SECURE_RANDOM_IO_ERROR;
}

/* end of sys_secure_random.c */
