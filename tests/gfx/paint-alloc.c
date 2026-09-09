/*
    FreeBASIC graphics tests: paint-alloc.c
    Fault injection for gfx_paint.c's private span allocations. Compile that
    translation unit with malloc/calloc/free redirected to these functions.
    Other runtime allocations and production allocator behavior are unchanged.
*/

#include <stdlib.h>

/* The fixture has one graphics caller. Only the instrumented object uses us. */
static int fail_at, allocations, outstanding;

void fb_test_paint_fail_at(int allocation)
{
	fail_at = allocation;
	allocations = 0;
}

int fb_test_paint_outstanding(void) { return outstanding; }

void *fb_test_paint_malloc(size_t size)
{
	void *memory;
	if (++allocations == fail_at) return NULL;
	memory = malloc(size);
	if (memory) outstanding++;
	return memory;
}

void *fb_test_paint_calloc(size_t count, size_t size)
{
	void *memory;
	if (++allocations == fail_at) return NULL;
	memory = calloc(count, size);
	if (memory) outstanding++;
	return memory;
}

void fb_test_paint_free(void *memory)
{
	if (memory) outstanding--;
	free(memory);
}

/* end of paint-alloc.c */
