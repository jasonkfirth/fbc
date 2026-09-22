/* FreeBASIC DOS runtime: thread_lock.c
 *
 * Serialize nonreentrant rtlib/CRT operations against preemption. DOS has one
 * CPU execution context, so the runtime's recursive locks share a scheduling
 * exclusion counter. Hardware interrupts continue, including Sound Blaster
 * DMA completion. Blocking thread operations must be outside these regions.
 * Public application mutexes are implemented separately in thread_mutex.c.
 */

#include "../fb.h"
#include "fb_dos_thread.h"

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

static unsigned int lock_depth;
static int saved_state;

int fb_DosThreadStarted( void )
{
	/* Initialization can be requested inside a runtime critical section.
	 * Enable preemption at its outermost unlock, never halfway through it.
	 */
	if( lock_depth ) {
		saved_state = 0;
		return 1;
	}
	return 0;
}

FBCALL void fb_Lock( void )
{
	int previous = fb_DosThreadEnter();
	assert( lock_depth != UINT_MAX );
	if( lock_depth++ == 0 )
		saved_state = previous;
}

FBCALL void fb_Unlock( void )
{
	/* IRQ callbacks can take this lock recursively. Once depth reaches zero
	 * an interrupt's own lock can replace saved_state, so capture the state
	 * before publishing that zero depth. Hardware IRQs remain enabled.
	 */
	int previous = saved_state;
	assert( lock_depth > 0 );
	if( --lock_depth == 0 )
		fb_DosThreadLeave( previous );
}

FBCALL void fb_StrLock( void ) { fb_Lock(); }
FBCALL void fb_StrUnlock( void ) { fb_Unlock(); }
FBCALL void fb_GraphicsLock( void ) { fb_Lock(); }
FBCALL void fb_GraphicsUnlock( void ) { fb_Unlock(); }
FBCALL void fb_MathLock( void ) { fb_Lock(); }
FBCALL void fb_MathUnlock( void ) { fb_Unlock(); }
FBCALL void fb_ProfileLock( void ) { fb_Lock(); }
FBCALL void fb_ProfileUnlock( void ) { fb_Unlock(); }

#endif

/* end of thread_lock.c */
