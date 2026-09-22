/* FreeBASIC DOS runtime: thread_memory.c
 *
 * Linker wrappers protect the DJGPP heap, including calls from precompiled
 * libraries. The compiler adds --wrap for these four C symbols only for
 * the explicit DOS thread profile. Restoring the previous scheduler state
 * keeps calls made within an existing runtime critical section safe.
 * This is not a replacement allocator or a general thread-safe DJGPP libc.
 */

#include "../fb.h"
#include "fb_dos_thread.h"

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

extern void *__real_malloc( size_t size );
extern void *__real_calloc( size_t count, size_t size );
extern void *__real_realloc( void *ptr, size_t size );
extern void __real_free( void *ptr );

void *__wrap_malloc( size_t size )
{
	int previous = fb_DosThreadEnter();
	void *result = __real_malloc( size );
	fb_DosThreadLeave( previous );
	return result;
}

void *__wrap_calloc( size_t count, size_t size )
{
	int previous = fb_DosThreadEnter();
	void *result = __real_calloc( count, size );
	fb_DosThreadLeave( previous );
	return result;
}

void *__wrap_realloc( void *ptr, size_t size )
{
	int previous = fb_DosThreadEnter();
	void *result = __real_realloc( ptr, size );
	fb_DosThreadLeave( previous );
	return result;
}

void __wrap_free( void *ptr )
{
	int previous = fb_DosThreadEnter();
	__real_free( ptr );
	fb_DosThreadLeave( previous );
}

#endif

/* end of thread_memory.c */
