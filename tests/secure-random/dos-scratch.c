/*
 * FreeBASIC DOS runtime tests: dos-scratch.c
 * Observe the real-mode staging buffer after a successful public API call.
 * The BASIC test runs without other I/O workers between the call and observer.
 * This test helper only reads memory; it does not supply or replace entropy.
 */

#include <go32.h>
#include <stddef.h>
#include <sys/movedata.h>

int TestSecureRandomScratchClear( size_t count )
{
	unsigned char bytes[256];
	size_t offset = 0;
	if( count < 16 )
		count = 16; /* The control reply occupies 16 bytes even for short reads. */
	if( count > 4096 || __tb_size < count || __tb > 0x100000UL - count )
		return 0;
	while( offset < count ) {
		size_t chunk = count - offset;
		size_t i;
		if( chunk > sizeof( bytes ) )
			chunk = sizeof( bytes );
		dosmemget( __tb + offset, chunk, bytes );
		for( i = 0; i < chunk; ++i ) {
			if( bytes[i] != 0 )
				return 0;
		}
		offset += chunk;
	}
	return 1;
}

/* end of dos-scratch.c */
