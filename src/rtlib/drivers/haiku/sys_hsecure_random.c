/*
 * Project: FreeBASIC runtime library
 * File: drivers/haiku/sys_hsecure_random.c
 * Purpose: Provide sys hsecure random runtime support.
 * Responsibilities: Target-specific runtime support for haiku.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
 * FreeBASIC Haiku runtime: sys_hsecure_random.c
 * Obtain checked hardware DRNG bytes on the qualified x86-64 platform.
 * CPUID is checked before RDRAND, every carry result is checked, and repeated
 * consecutive 64-bit output words fail the request. All state is call-local.
 * This file does not seed a software generator or fall back to timer entropy.
 */

#include "../../fb.h"
#include "../../fb_secure_random.h"

#if defined(__x86_64__)
#include <cpuid.h>

static int hReadWord( uint64_t *word )
{
	unsigned int attempt;
	for( attempt = 0; attempt < 10; ++attempt ) {
		unsigned char ready;
		uint64_t value;
		/* Intel's DRNG contract publishes output only with carry set.
		 * Ten retries bound a busy or unhealthy device. This instruction
		 * is reached only after the caller checks CPUID leaf 1, ECX bit 30.
		 */
		__asm__ __volatile__( "rdrand %0; setc %1"
			: "=r"(value), "=qm"(ready) : : "cc" );
		if( ready ) {
			*word = value;
			return FB_SECURE_RANDOM_OK;
		}
	}
	return FB_SECURE_RANDOM_IO_ERROR;
}
#endif

int fb_hSecureRandom( void *destination, size_t byte_count )
{
#if defined(__x86_64__)
	unsigned int eax, ebx, ecx, edx;
	unsigned char *cursor = destination;
	uint64_t previous;
	if( !__get_cpuid( 1, &eax, &ebx, &ecx, &edx ) ||
	    (ecx & (1U << 30)) == 0 )
		return FB_SECURE_RANDOM_UNAVAILABLE;
	if( hReadWord( &previous ) != FB_SECURE_RANDOM_OK )
		return FB_SECURE_RANDOM_IO_ERROR;
	while( byte_count != 0 ) {
		uint64_t value;
		size_t chunk = byte_count > sizeof( value ) ? sizeof( value ) : byte_count;
		if( hReadWord( &value ) != FB_SECURE_RANDOM_OK || value == previous )
			return FB_SECURE_RANDOM_IO_ERROR;
		/* Compare full words even for a one-byte request. A discarded first
		 * word lets short requests detect a stuck source without global state.
		 * This detects repeats; it is not an entropy estimate or attestation.
		 */
		memcpy( cursor, &value, chunk );
		previous = value;
		cursor += chunk;
		byte_count -= chunk;
	}
	return FB_SECURE_RANDOM_OK;
#else
	(void)destination;
	(void)byte_count;
	return FB_SECURE_RANDOM_UNAVAILABLE;
#endif
}

/* end of drivers/haiku/sys_hsecure_random.c */
