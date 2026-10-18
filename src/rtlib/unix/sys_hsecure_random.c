/*
 * FreeBASIC Unix runtime: sys_hsecure_random.c
 * Read checked system randomness on Linux/Android, supported FreeBSD and
 * Darwin. Nonblocking kernel requests report an unready pool instead of
 * stalling startup. Other Unix targets require a source override, not a weak
 * fallback.
 */

#include "../fb.h"
#include "../fb_secure_random.h"
#include <errno.h>
#include <unistd.h>

#if defined(HOST_LINUX) || defined(HOST_ANDROID)
	#include <sys/syscall.h>
	#if defined(SYS_getrandom)
		#define FB_HAVE_CHECKED_GETRANDOM
		/* Linux getrandom(2) flag: fail with EAGAIN until the pool is ready. */
		#define FB_GETRANDOM_NONBLOCK 1U
	#endif
#elif defined(HOST_FREEBSD) && __FreeBSD__ >= 12
	#include <sys/random.h>
	#define FB_HAVE_CHECKED_GETRANDOM
	#define FB_GETRANDOM_NONBLOCK GRND_NONBLOCK
#elif defined(HOST_DARWIN)
	#include <stdlib.h>
	#define FB_HAVE_ARC4RANDOM_BUF
#endif

int fb_hSecureRandom( void *destination, size_t byte_count )
{
#ifdef FB_HAVE_CHECKED_GETRANDOM
	unsigned char *cursor = destination;
	unsigned int interruptions = 0;
	while( byte_count != 0 ) {
		/* Small kernel requests avoid large partial transfers. The public
		 * one-MiB bound also bounds the number of successful iterations.
		 */
		size_t chunk = byte_count > 256 ? 256 : byte_count;
		ssize_t count;
		#if defined(HOST_LINUX) || defined(HOST_ANDROID)
			count = syscall( SYS_getrandom, cursor, chunk, FB_GETRANDOM_NONBLOCK );
		#else
			count = getrandom( cursor, chunk, FB_GETRANDOM_NONBLOCK );
		#endif
		if( count < 0 ) {
			if( errno == EINTR && ++interruptions <= 16 )
				continue;
			if( errno == EAGAIN || errno == ENOSYS )
				return FB_SECURE_RANDOM_UNAVAILABLE;
			return FB_SECURE_RANDOM_IO_ERROR;
		}
		if( count == 0 || (size_t)count > chunk )
			return FB_SECURE_RANDOM_IO_ERROR;
		cursor += count;
		byte_count -= (size_t)count;
	}
	return FB_SECURE_RANDOM_OK;
#elif defined(FB_HAVE_ARC4RANDOM_BUF)
	/* Darwin libc fills the complete span and exposes no per-call status. */
	arc4random_buf( destination, byte_count );
	return FB_SECURE_RANDOM_OK;
#else
	(void)destination;
	(void)byte_count;
	return FB_SECURE_RANDOM_UNAVAILABLE;
#endif
}

/* end of sys_hsecure_random.c */
