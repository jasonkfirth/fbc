/*
 * FreeBASIC runtime: js/sys_hsecure_random.c
 * Purpose: Obtain checked system randomness from browser Web Crypto.
 * Responsibilities: Bounded heap writes, quota-sized requests and failure status.
 * This file does not validate the public API or substitute a user-seeded PRNG.
 * Threading: Each call owns scratch storage; the current browser/worker owns its heap.
 */
#include "../../fb.h"
#include "../../fb_secure_random.h"
#include <emscripten.h>

/* Internal results are separate from the public C enumeration: 1 filled,
 * 0 unavailable, -1 failed. The C wrapper supplies the public status codes. */
EM_JS(int, js_secure_random, (void *destination, size_t byte_count), {
	var provider = typeof globalThis !== 'undefined' ? globalThis.crypto : null;
	if (!provider || typeof provider.getRandomValues !== 'function') return 0;
	var start = destination >>> 0;
	var count = byte_count >>> 0;
	if (start > HEAPU8.length || count > HEAPU8.length - start) return -1;
	var scratch;
	try {
		/* Web Crypto limits one request to 65,536 bytes. A separate ordinary
		 * buffer also works when a threaded Wasm heap uses SharedArrayBuffer. */
		scratch = new Uint8Array(Math.min(count, 65536));
		for (var offset = 0; offset < count; offset += scratch.length) {
			var chunk = scratch.subarray(0, Math.min(scratch.length, count - offset));
			provider.getRandomValues(chunk);
			HEAPU8.set(chunk, start + offset);
		}
		return 1;
	} catch (error) {
		return -1;
	} finally {
		if (scratch) scratch.fill(0);
	}
});

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	int result = js_secure_random( destination, byte_count );
	if( result == 1 )
		return FB_SECURE_RANDOM_OK;
	if( result == 0 )
		return FB_SECURE_RANDOM_UNAVAILABLE;
	return FB_SECURE_RANDOM_IO_ERROR;
}

/* end of js/sys_hsecure_random.c */
