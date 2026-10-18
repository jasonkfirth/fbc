/*
 * FreeBASIC runtime: fb_secure_random.h
 * Checked system-randomness status, bounds and entry points.
 * The public call owns validation and failure clearing; platform helpers own
 * system access. This header contains no generator or process-wide RNG state.
 */
#ifndef __FB_SECURE_RANDOM_H__
#define __FB_SECURE_RANDOM_H__

#include "fb.h"

enum {
	FB_SECURE_RANDOM_OK = 0,
	FB_SECURE_RANDOM_UNAVAILABLE = 1,
	FB_SECURE_RANDOM_IO_ERROR = 2,
	FB_SECURE_RANDOM_INVALID_ARGUMENT = 3
};

/* One MiB bounds every request and its complete failure wipe. */
#define FB_SECURE_RANDOM_MAX_BYTES ((size_t)1048576)

#ifdef __cplusplus
extern "C" {
#endif

FBCALL int fb_SecureRandom( void *destination, size_t byte_count );

/* Runtime-private provider. It must return OK only after filling every byte.
 * It may leave partial output on failure; the public call clears that span.
 * No provider may substitute a seeded PRNG when its system source fails.
 */
int fb_hSecureRandom( void *destination, size_t byte_count );

#ifdef __cplusplus
}
#endif

#endif
/* end of fb_secure_random.h */
