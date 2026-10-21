/*
 * Project: FreeBASIC graphics regression tests
 * File: locking.c
 * Purpose: Exercise input callbacks, allocation and shutdown with real locks.
 * Responsibilities: Detect reversed locks and incomplete mode cleanup.
 * This file intentionally does NOT require a display or synthesize native input.
 */

#include <pthread.h>
#include <stdatomic.h>
#include "../../src/gfxlib2/fb_gfx.h"

static _Thread_local int runtime_depth, graphics_depth;
static atomic_int fail_allocation, wait_for_runtime, callbacks, exits;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int reader_ready, runtime_attempted;
static GFXDRIVER driver;

/* The runner distinguishes this assertion exit from a crash or tool failure. */
#define TEST_FAILURE 88

FBCALL void __real_fb_Lock(void);
FBCALL void __real_fb_Unlock(void);
FBCALL void __real_fb_GraphicsLock(void);
FBCALL void __real_fb_GraphicsUnlock(void);
void *__real_calloc(size_t, size_t);

static void check(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "GFX_LOCKING_FAIL %s\n", message);
		exit(TEST_FAILURE);
	}
}

FBCALL void __wrap_fb_Lock(void)
{
	check(!graphics_depth || runtime_depth, "runtime lock entered after graphics");
	if (atomic_load(&wait_for_runtime) && !runtime_depth) {
		pthread_mutex_lock(&gate);
		runtime_attempted = 1;
		pthread_cond_broadcast(&changed);
		pthread_mutex_unlock(&gate);
	}
	__real_fb_Lock();
	++runtime_depth;
}

FBCALL void __wrap_fb_Unlock(void)
{
	check(runtime_depth > 0, "unbalanced runtime unlock");
	--runtime_depth;
	__real_fb_Unlock();
}

FBCALL void __wrap_fb_GraphicsLock(void)
{
	__real_fb_GraphicsLock();
	++graphics_depth;
}

FBCALL void __wrap_fb_GraphicsUnlock(void)
{
	check(graphics_depth > 0, "unbalanced graphics unlock");
	--graphics_depth;
	__real_fb_GraphicsUnlock();
}

void *__wrap_calloc(size_t count, size_t bytes)
{
	if (atomic_exchange(&fail_allocation, 0))
		return NULL;
	return __real_calloc(count, bytes);
}

static void poll_events(void)
{
	check(runtime_depth && graphics_depth, "input callback lost its mode locks");
	DRIVER_LOCK();
	fb_hPostKey('K');
	DRIVER_UNLOCK();
	atomic_fetch_add(&callbacks, 1);
}

static void driver_exit(void)
{
	check(runtime_depth && graphics_depth, "driver exit lost its mode locks");
	check(__fb_gfx && __fb_gfx->page && __fb_gfx->event_mutex,
	      "mode storage was freed before driver exit");
	atomic_fetch_add(&exits, 1);
}

static void *reader(void *unused)
{
	FBSTRING *key;
	(void)unused;
	FB_LOCK();
	pthread_mutex_lock(&gate);
	reader_ready = 1;
	pthread_cond_broadcast(&changed);
	while (!runtime_attempted)
		pthread_cond_wait(&changed, &gate);
	pthread_mutex_unlock(&gate);
	/* The mode-changing thread is now trying to acquire FB_LOCK. It must
	 * not already own graphics, or this real hook call would deadlock.
	 */
	key = fb_Inkey();
	check(FB_STRSIZE(key) == 1 && key->data[0] == 'K', "input string allocation failed");
	fb_hStrDelTemp(key);
	FB_UNLOCK();
	fb_TlsFreeCtxTb();
	return NULL;
}

int main(void)
{
	pthread_t thread;
	unsigned char input[] = "graphics codec";
	unsigned char encoded[128], decoded[128];
	ssize_t encoded_size = sizeof(encoded), decoded_size = sizeof(decoded);
	int result;

	fb_hRtInit();
	check(fb_GfxScreenRes(64, 64, 32, 1, DRIVER_NULL, 0) == FB_RTERROR_OK, "initial mode failed");
	driver = *__fb_gfx->driver;
	driver.poll_events = poll_events;
	driver.exit = driver_exit;
	__fb_gfx->driver = &driver;
	check(pthread_create(&thread, NULL, reader, NULL) == 0, "reader launch failed");
	pthread_mutex_lock(&gate);
	while (!reader_ready)
		pthread_cond_wait(&changed, &gate);
	pthread_mutex_unlock(&gate);
	atomic_store(&wait_for_runtime, 1);
	result = fb_GfxScreenRes(64, 64, 32, 1, DRIVER_NULL, 0);
	check(pthread_join(thread, NULL) == 0 && result == FB_RTERROR_OK, "mode/input interleave failed");
	atomic_store(&wait_for_runtime, 0);
	check(atomic_load(&callbacks) == 1 && atomic_load(&exits) == 1, "callback/teardown path was not exercised");

	/* The historical codec entry points can run with only graphics held.
	 * Their shared table must not introduce a hidden runtime-lock acquisition.
	 */
	FB_GRAPHICS_LOCK();
	check(fb_hEncode(input, sizeof(input), encoded, &encoded_size) == 0, "encode failed");
	check(fb_hDecode(encoded, encoded_size, decoded, &decoded_size) == 0
	      && decoded_size == sizeof(input) && memcmp(input, decoded, sizeof(input)) == 0, "decode failed");
	FB_GRAPHICS_UNLOCK();

	atomic_store(&fail_allocation, 1);
	check(fb_GfxScreenRes(64, 64, 32, 1, DRIVER_NULL, 0) == FB_RTERROR_OUTOFMEM,
	      "allocation failure was not exercised");
	check(!__fb_gfx && !__fb_ctx.hooks.inkeyproc, "failed allocation left mode/hooks installed");
	check(fb_GfxScreenQB(-1, -1, -1) == FB_RTERROR_ILLEGALFUNCTIONCALL, "invalid QB mode accepted");
	check(!runtime_depth && !graphics_depth, "failure path leaked a lock");
	check(fb_GfxScreenRes(64, 64, 32, 1, DRIVER_NULL, 0) == FB_RTERROR_OK, "mode recovery failed");
	__fb_gfx->driver = &driver;
	__fb_ctx.exit_gfxlib2();
	__fb_ctx.exit_gfxlib2();
	check(!__fb_gfx && !__fb_ctx.hooks.inkeyproc && atomic_load(&exits) == 2,
	      "shutdown was incomplete or not idempotent");
	check(!runtime_depth && !graphics_depth, "shutdown leaked a lock");
	fb_hRtExit();
	puts("GFX_LOCKING_PASS input=1 allocation_failure=1 codec=1 shutdown=2");
	return 0;
}

/* end of locking.c */
