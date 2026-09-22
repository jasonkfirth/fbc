/* FreeBASIC DOS runtime: fb_dos_thread.h
 *
 * Private interface to the opt-in PDMLWP scheduler. Critical sections prevent
 * task switches, not hardware interrupts. They protect DJGPP's nonreentrant
 * runtime and must not contain a blocking thread operation. This header does
 * not expose a public pthread ABI or change the default DOS runtime.
 */

#ifndef FB_DOS_THREAD_H
#define FB_DOS_THREAD_H

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)
#include <assert.h>
#include <limits.h>
#include "../../../contrib/dos/pdmlwp/include/lwp.h"

extern volatile int _lwp_enable;
extern volatile int _lwp_reschedule;
extern volatile int _lwp_interrupt_pending;

static __inline__ int fb_DosThreadEnter( void )
{
	int previous = 1;
	/* The exchange closes the interrupt window between reading and disabling
	 * the scheduler. Returning its old state makes nested calls safe.
	 */
	__asm__ __volatile__( "xchgl %0, %1"
		: "+r"(previous), "+m"(_lwp_enable) : : "memory" );
	return previous;
}

static __inline__ void fb_DosThreadLeave( int previous )
{
	__asm__ __volatile__( "" : : : "memory" );
	_lwp_enable = previous;
	/* A tick deferred by a critical section must get a scheduling opportunity
	 * at its outermost unlock. Otherwise frequent short clock/heap calls can
	 * exclude every timer tick and starve other threads. Never yield from
	 * inside the provider's own dispatch or a still-pending synthetic signal.
	 */
	if( !previous && _lwp_reschedule && !_lwp_interrupt_pending && _lwp_cur )
		lwp_yield();
}

int fb_DosThreadInit( void );
int fb_DosThreadStarted( void );
void *fb_DosTlsGet( int index );
int fb_DosTlsSet( int index, void *value );
void fb_DosThreadExit( void );
/* Sleep without invoking sfxlib, including from inside a driver write. */
void fb_DosThreadDelay( unsigned int msecs );

#endif
#endif

/* end of fb_dos_thread.h */
