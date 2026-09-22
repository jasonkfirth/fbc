/* FreeBASIC DOS runtime: thread_clock.c
 *
 * Serialize DJGPP uclock() calls in the optional thread profile. Its PIT
 * latch reads and midnight rollover bookkeeping are shared across callers.
 * Preemption after reading the BIOS tick but before updating last_tics can
 * otherwise add a spurious day. Hardware IRQs remain enabled during the call;
 * this file does not reprogram timers or replace the CRT clock algorithm.
 */

#include "../fb.h"
#include "fb_dos_thread.h"
#include <time.h>

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

extern uclock_t __real_uclock( void );

uclock_t __wrap_uclock( void )
{
	int previous = fb_DosThreadEnter();
	uclock_t result = __real_uclock();
	fb_DosThreadLeave( previous );
	return result;
}

#endif

/* end of thread_clock.c */
