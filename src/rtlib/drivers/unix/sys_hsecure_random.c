/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/sys_hsecure_random.c
 * Purpose: Provide sys hsecure random runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
 * FreeBASIC Unix runtime: sys_hsecure_random.c
 * Read checked system randomness on Linux/Android, supported FreeBSD, Darwin,
 * OpenBSD, DragonFlyBSD and NetBSD 6.0 or later. Nonblocking kernel requests
 * report an unready pool instead of stalling startup. Other Unix targets
 * require a source override, not a weak fallback.
 */

#include "../../fb.h"
#include "../../fb_secure_random.h"
#include <errno.h>
#include <unistd.h>

#ifdef HOST_ANDROID
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/system_properties.h>

static int hAndroidDeviceRandom( void *destination, size_t byte_count )
{
	char boot_completed[PROP_VALUE_MAX];
	unsigned char *cursor = destination;
	unsigned int interruptions = 0;
	struct stat information;
	int descriptor;
	int status = FB_SECURE_RANDOM_IO_ERROR;

	/* Android 7 can run a 3.10 kernel without getrandom(2). Its platform
	 * seeds the kernel device before completing boot, as Bionic's own random
	 * provider requires. Refuse that fallback during early boot. Other Unix
	 * platforms retain their existing getrandom readiness contract. */
	if( __system_property_get( "sys.boot_completed", boot_completed ) != 1 || boot_completed[0] != '1' )
		return FB_SECURE_RANDOM_UNAVAILABLE;
	do {
		descriptor = open( "/dev/urandom", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK );
	} while( descriptor < 0 && errno == EINTR && ++interruptions <= 16 );
	if( descriptor < 0 )
		return FB_SECURE_RANDOM_IO_ERROR;
	if( fstat( descriptor, &information ) != 0 || !S_ISCHR( information.st_mode ) ) {
		close( descriptor );
		return FB_SECURE_RANDOM_IO_ERROR;
	}
	while( byte_count != 0 ) {
		size_t chunk = byte_count > 256 ? 256 : byte_count;
		ssize_t count = read( descriptor, cursor, chunk );
		if( count < 0 ) {
			if( errno == EINTR && ++interruptions <= 16 )
				continue;
			break;
		}
		if( count == 0 || (size_t)count > chunk )
			break;
		cursor += count;
		byte_count -= (size_t)count;
	}
	if( byte_count == 0 )
		status = FB_SECURE_RANDOM_OK;
	if( close( descriptor ) != 0 )
		status = FB_SECURE_RANDOM_IO_ERROR;
	return status;
}
#endif

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
#elif defined(HOST_NETBSD)
	#include <sys/param.h>
	#if defined(__NetBSD_Version__) && __NetBSD_Version__ >= 600000000
		#include <stdlib.h>
		#define FB_HAVE_ARC4RANDOM_BUF
	#endif
#elif defined(HOST_DARWIN) || defined(HOST_OPENBSD) || defined(HOST_DRAGONFLY)
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
			#ifdef HOST_ANDROID
				if( errno == ENOSYS )
					return hAndroidDeviceRandom( cursor, byte_count );
			#endif
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
#elif defined(HOST_ANDROID)
	return hAndroidDeviceRandom( destination, byte_count );
#elif defined(FB_HAVE_ARC4RANDOM_BUF)
	/* These libc APIs fill the span and expose no per-call status. */
	arc4random_buf( destination, byte_count );
	return FB_SECURE_RANDOM_OK;
#else
	(void)destination;
	(void)byte_count;
	return FB_SECURE_RANDOM_UNAVAILABLE;
#endif
}

/* end of drivers/unix/sys_hsecure_random.c */
