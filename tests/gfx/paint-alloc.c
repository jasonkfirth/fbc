/*
    FreeBASIC graphics tests: paint-alloc.c
    Fault injection for gfx_paint.c's private span allocations. Source-tree
    tests redirect that translation unit at compile time. Installed-package
    tests use the linker's symbol wrapping because rtlib source is not staged.
*/

#include <stdlib.h>

/* The fixture has one graphics caller. Only the instrumented object uses us. */
static int fail_at = -1;
static int allocations, outstanding;

#ifdef FB_TEST_LINKER_WRAP
extern void *__real_malloc(size_t size);
extern void *__real_calloc(size_t count, size_t size);
extern void __real_free(void *memory);
#define TEST_MALLOC __real_malloc
#define TEST_CALLOC __real_calloc
#define TEST_FREE __real_free
#define TEST_MALLOC_NAME __wrap_malloc
#define TEST_CALLOC_NAME __wrap_calloc
#define TEST_FREE_NAME __wrap_free
#else
#define TEST_MALLOC malloc
#define TEST_CALLOC calloc
#define TEST_FREE free
#define TEST_MALLOC_NAME fb_test_paint_malloc
#define TEST_CALLOC_NAME fb_test_paint_calloc
#define TEST_FREE_NAME fb_test_paint_free
#endif

void fb_test_paint_fail_at(int allocation)
{
	fail_at = allocation;
	allocations = 0;
}

int fb_test_paint_outstanding(void)
{
	int result = outstanding;
	fail_at = -1;
	return result;
}

void *TEST_MALLOC_NAME(size_t size)
{
	void *memory;
	if (fail_at >= 0 && ++allocations == fail_at) return NULL;
	memory = TEST_MALLOC(size);
	if (memory && fail_at >= 0) outstanding++;
	return memory;
}

void *TEST_CALLOC_NAME(size_t count, size_t size)
{
	void *memory;
	if (fail_at >= 0 && ++allocations == fail_at) return NULL;
	memory = TEST_CALLOC(count, size);
	if (memory && fail_at >= 0) outstanding++;
	return memory;
}

void TEST_FREE_NAME(void *memory)
{
	if (memory && fail_at >= 0) outstanding--;
	TEST_FREE(memory);
}

#undef TEST_MALLOC
#undef TEST_CALLOC
#undef TEST_FREE
#undef TEST_MALLOC_NAME
#undef TEST_CALLOC_NAME
#undef TEST_FREE_NAME

/* end of paint-alloc.c */
