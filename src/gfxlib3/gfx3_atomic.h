/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_atomic.h

    Purpose:

        Provide atomic operations for compilers that predate C11 atomics.

    Responsibilities:

        - use the standard atomic API when the compiler provides it
        - map the small gfxlib3 atomic surface to GCC synchronization builtins
          on older GCC toolchains

    This file intentionally does NOT contain:

        - locks for command queues or resource registries
        - platform thread creation
        - atomic objects with sizes beyond int and uint64_t
*/

#ifndef FB_GFX3_ATOMIC_H
#define FB_GFX3_ATOMIC_H

#include <stdint.h>

#if defined(__clang__) || \
	(defined(__GNUC__) && ((__GNUC__ > 4) || \
	 ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9))))
#include <stdatomic.h>
#else

/* GCC 4.7 in GCCSDK has __sync builtins but no stdatomic.h. */
#define _Atomic volatile

#define memory_order_relaxed 0
#define memory_order_consume 1
#define memory_order_acquire 2
#define memory_order_release 3
#define memory_order_acq_rel 4
#define memory_order_seq_cst 5

#define atomic_init(object, value) (*(object) = (value))
#define atomic_load(object) \
	__sync_fetch_and_add((object), (__typeof__(*(object)))0)
#define atomic_load_explicit(object, order) \
	((void)(order), __sync_fetch_and_add((object), (__typeof__(*(object)))0))
#define atomic_store(object, value) \
	((void)__sync_lock_test_and_set((object), (value)))
#define atomic_store_explicit(object, value, order) \
	((void)(order), (void)__sync_lock_test_and_set((object), (value)))
#define atomic_fetch_add(object, value) \
	__sync_fetch_and_add((object), (value))
#define atomic_fetch_add_explicit(object, value, order) \
	((void)(order), __sync_fetch_and_add((object), (value)))

static __inline__ int fb_gfx3_atomic_compare_exchange_int(
	volatile int *object, int *expected, int desired)
{
	int previous = __sync_val_compare_and_swap(object, *expected, desired);

	if (previous == *expected)
		return 1;
	*expected = previous;
	return 0;
}

#define atomic_compare_exchange_strong_explicit( \
	object, expected, desired, success_order, failure_order) \
	((void)(success_order), (void)(failure_order), \
	 fb_gfx3_atomic_compare_exchange_int((object), (expected), (desired)))

#endif

#endif

/* end of gfx3_atomic.h */
