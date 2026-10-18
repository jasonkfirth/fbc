/*
 * FreeBASIC runtime: sys_hsecure_random.c
 * Refuse checked entropy on targets without a qualified system provider.
 * Target source overrides supply real system bytes. This fallback does not
 * seed a PRNG, touch caller memory or silently promote RND to secure output.
 */

#include "fb.h"
#include "fb_secure_random.h"

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	(void)destination;
	(void)byte_count;
	return FB_SECURE_RANDOM_UNAVAILABLE;
}

/* end of sys_hsecure_random.c */
