/*
 * Project: FreeBASIC gfxlib2 regression tests
 * File: screenlist-threads.c
 * Purpose: Check that SCREENLIST walks belong to the thread that starts them.
 * Responsibilities: Provide fixed driver modes and interleave two callers.
 * This file intentionally does NOT contain: Display setup or driver internals.
 */
#include "../../src/gfxlib2/fb_gfx.h"

#define MODE(w, h) (((w) << 16) | (h))

static FBMUTEX *gate;
static FBCOND *changed;
static int worker_ready, main_done;

static void check(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "SCREENLIST: %s\n", message);
		exit(1);
	}
}

static int *copy_modes(const int *source, int count, int *size)
{
	int *result = (int *)malloc((size_t)count * sizeof(*result));
	check(result != NULL, "could not allocate mock driver modes");
	memcpy(result, source, (size_t)count * sizeof(*result));
	*size = count;
	return result;
}

static int *driver_modes(int depth, int *size)
{
	static const int modes16[] = { MODE(800, 600), MODE(640, 480), MODE(640, 400) };
	static const int modes32[] = { MODE(1920, 1080), MODE(1024, 768), MODE(1280, 720) };
	switch (depth) {
	case 16: return copy_modes(modes16, 3, size);
	case 32: return copy_modes(modes32, 3, size);
	default: *size = 0; return NULL;
	}
}

static int *duplicate_modes(int depth, int *size)
{
	static const int duplicate[] = { MODE(640, 480) };
	if (depth == 16)
		return copy_modes(duplicate, 1, size);
	*size = 0;
	return NULL;
}

static const GFXDRIVER primary_driver = { .fetch_modes = driver_modes };
static const GFXDRIVER secondary_driver = { .fetch_modes = duplicate_modes };
const GFXDRIVER *fb_gfx_drivers_list[] = { &primary_driver, &secondary_driver, NULL };

static void FBCALL worker(void *parameter)
{
	(void)parameter;
	check(fb_GfxScreenList(0) == 0, "worker inherited the main thread's walk");
	check(fb_GfxScreenList(32) == MODE(1024, 768), "worker did not start at the first 32-bit mode");

	fb_MutexLock(gate);
	worker_ready = 1;
	fb_CondSignal(changed);
	while (!main_done)
		fb_CondWait(changed, gate);
	fb_MutexUnlock(gate);

	check(fb_GfxScreenList(0) == MODE(1280, 720), "main thread advanced the worker's cursor");
	/* Leave a mode unread to exercise cleanup when a worker exits. */
}

int main(void)
{
	FBTHREAD *thread;

	fb_hRtInit();
	gate = fb_MutexCreate();
	changed = fb_CondCreate();
	check(gate != NULL && changed != NULL, "could not create the test gates");

	check(fb_GfxScreenList(0) == 0, "continuation before a start returned a mode");
	check(fb_GfxScreenList(16) == MODE(640, 400), "main did not start at the first 16-bit mode");
	thread = fb_ThreadCreate(worker, NULL, 0);
	check(thread != NULL, "could not start the worker");

	fb_MutexLock(gate);
	while (!worker_ready)
		fb_CondWait(changed, gate);

	check(fb_GfxScreenList(0) == MODE(640, 480), "worker replaced the main thread's walk");
	check(fb_GfxScreenList(0) == MODE(800, 600), "driver duplicates were not removed");
	check(fb_GfxScreenList(0) == 0 && fb_GfxScreenList(0) == 0, "exhausted walk returned a mode");
	check(fb_TlsGetCtxExisting(FB_TLSKEY_SCREENLIST) == NULL, "exhausted walk kept its storage");
	check(fb_GfxScreenList(16) == MODE(640, 400), "restart after exhaustion failed");
	check(fb_GfxScreenList(15) == 0 && fb_GfxScreenList(0) == 0, "empty walk kept old modes");

	main_done = 1;
	fb_CondSignal(changed);
	fb_MutexUnlock(gate);
	fb_ThreadWait(thread);
	fb_CondDestroy(changed);
	fb_MutexDestroy(gate);
	puts("GFX_SCREENLIST_THREADS_PASS");
	return 0;
}

/* end of tests/gfxlib2/screenlist-threads.c */
