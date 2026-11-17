/*
 * Project: FreeBASIC Wii checked system randomness
 * File: drivers/wii/sys_hsecure_random.c
 * Purpose: Obtain host system entropy through the versioned Dolphin IOS extension.
 * Responsibilities: Check the provider contract, bound IPC transfers and wipe scratch.
 * This file does not generate bytes from clocks, RND, save files or network peers.
 *
 * Stock Dolphin and physical consoles without this extension report unavailable.
 * The public fb_SecureRandom owner validates arguments and clears caller output
 * after any failure, including a failure following earlier successful chunks.
 * Each call owns its IOS descriptor and DMA-aligned storage; no state is shared
 * between runtime threads. IOS handles cache synchronization for the ioctl vectors.
 */
#include "../../fb.h"
#include "../../fb_secure_random.h"
#include <malloc.h>
#include <ogc/cache.h>
#include <ogc/ipc.h>

/* Private extension IDs use an XL namespace rather than the next upstream ID. */
#define FB_WII_RNG_CAPABILITIES 0x584c5201
#define FB_WII_RNG_READ         0x584c5202
#define FB_WII_RNG_CHUNK        4096

static void wipe_scratch( void *memory, size_t length )
{
	volatile unsigned char *bytes = memory;
	size_t remaining = length;
	while( remaining != 0 ) {
		*bytes++ = 0;
		remaining--;
	}
	/* IOS uses physical memory. Publish the wipe before releasing the buffer. */
	DCFlushRange( memory, length );
}

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	/* 0..7 magic/version; 8..9 big-endian maximum read; 10..11 flags
	 * (bit zero = host system CSPRNG); 12..15 reserved, zero. */
	static const unsigned char capabilities[16] = {
		'X','L','R','N','G','0','0','1', 16,0, 0,1, 0,0,0,0
	};
	/* Reserve a complete DMA cache line so IOS vector invalidation cannot
	 * discard another local variable sharing the unused part of that line. */
	ioctlv vectors[4] ATTRIBUTE_ALIGN(32);
	ioctlv *vector = &vectors[0];
	_Static_assert( sizeof( vectors ) == 32, "Wii IOS vector cache-line layout" );
	unsigned char *scratch;
	unsigned char *cursor = destination;
	s32 descriptor;
	int result = FB_SECURE_RANDOM_UNAVAILABLE;
	size_t amount;

	descriptor = IOS_Open( "/dev/dolphin", IPC_OPEN_NONE );
	if( descriptor < 0 )
		return result;
	scratch = memalign( 32, FB_WII_RNG_CHUNK );
	if( scratch == NULL ) {
		IOS_Close( descriptor );
		return FB_SECURE_RANDOM_IO_ERROR;
	}
	memset( scratch, 0, FB_WII_RNG_CHUNK );
	memset( vectors, 0, sizeof( vectors ) );
	vector->data = scratch;
	vector->len = sizeof( capabilities );
	if( IOS_Ioctlv( descriptor, FB_WII_RNG_CAPABILITIES, 0, 1, vector ) != 0 ||
	    memcmp( scratch, capabilities, sizeof( capabilities ) ) != 0 )
		goto finished;
	result = FB_SECURE_RANDOM_IO_ERROR;
	while( byte_count != 0 ) {
		amount = byte_count > FB_WII_RNG_CHUNK ? FB_WII_RNG_CHUNK : byte_count;
		/* libogc converts ioctl vector addresses for IOS; rebuild each request. */
		vector->data = scratch;
		vector->len = amount;
		if( IOS_Ioctlv( descriptor, FB_WII_RNG_READ, 0, 1, vector ) != 0 )
			goto finished;
		memcpy( cursor, scratch, amount );
		wipe_scratch( scratch, FB_WII_RNG_CHUNK );
		cursor += amount;
		byte_count -= amount;
	}
	result = FB_SECURE_RANDOM_OK;
finished:
	wipe_scratch( scratch, FB_WII_RNG_CHUNK );
	free( scratch );
	if( IOS_Close( descriptor ) < 0 && result == FB_SECURE_RANDOM_OK )
		result = FB_SECURE_RANDOM_IO_ERROR;
	return result;
}
/* end of drivers/wii/sys_hsecure_random.c */
