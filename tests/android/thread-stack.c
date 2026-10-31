/* Project: FreeBASIC Android runtime; File: thread-stack.c.
 * Purpose: Verify the real ThreadCreate stack reserve on an Android device.
 * Responsibilities: Default/minimum/explicit sizes, usable stack and refusal.
 * This test does not create windows, use JNI or retain worker allocations. */
#define _GNU_SOURCE
#include "fb.h"
#include <assert.h>
#include <limits.h>
#include <pthread.h>

struct result { size_t minimum; int completed; };

static void worker(void *parameter)
{
	struct result *result = parameter;
	pthread_attr_t attributes;
	void *base = NULL;
	size_t bytes = 0;
	/* Exceed the former 16 KiB Android minimum using actual stack pages. */
	volatile unsigned char scratch[65536];
	for (size_t at = 0; at < sizeof(scratch); at += 4096)
		scratch[at] = (unsigned char)(at / 4096);
	assert(pthread_getattr_np(pthread_self(), &attributes) == 0);
	assert(pthread_attr_getstack(&attributes, &base, &bytes) == 0);
	assert(pthread_attr_destroy(&attributes) == 0);
	assert(base != NULL && bytes >= result->minimum);
	for (size_t at = 0; at < sizeof(scratch); at += 4096)
		assert(scratch[at] == (unsigned char)(at / 4096));
	result->completed = 1;
}

int main(int argc, char **argv)
{
	const ssize_t requested[] = {0, 1, -1, 16 * 1024 * 1024};
	fb_Init(argc, argv, 0);
	for (size_t index = 0; index < sizeof(requested) / sizeof(requested[0]); ++index) {
		struct result result = {index == 3 ? 16 * 1024 * 1024 : 8 * 1024 * 1024, 0};
		FBTHREAD *thread = fb_ThreadCreate(worker, &result, requested[index]);
		assert(thread != NULL);
		fb_ThreadWait(thread);
		assert(result.completed == 1);
	}
	/* An impossible reserve must fail rather than run on a smaller default. */
	assert(fb_ThreadCreate(worker, NULL, SSIZE_MAX) == NULL);
	puts("ANDROID_THREAD_STACK=PASS cases=5");
	fb_End(0);
	return 0;
}
/* end of thread-stack.c */
