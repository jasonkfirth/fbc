/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/thread_delay.c
 * Purpose: Provide thread delay runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/* FreeBASIC DOS runtime: thread_delay.c
 *
 * Preserve RTC ownership when an external library calls DJGPP delay(). The
 * original uses BIOS INT 15h/AH=86h, which reprograms IRQ8 and disables its
 * periodic interrupt on completion. That would silently stop preemption.
 * Sleep through the scheduler when possible, otherwise read the PIT clock.
 * This also works before initialization and inside a critical section.
 * Hardware interrupts remain in their caller's state. Do not invoke the audio idle
 * hook, since callers include the Sound Blaster reset/initialization path.
 */

#include "../../fb.h"
#include "fb_dos_thread.h"
#include <time.h>

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

void fb_DosThreadDelay( unsigned int msecs )
{
	if( _lwp_cur && !_lwp_enable ) {
		if( msecs )
			lwp_sleep( msecs / 1000, msecs % 1000 );
		else
			lwp_yield();
		return;
	}
	uclock_t started = uclock();
	/* uclock_t is 64-bit on DJGPP; widen before multiplying and round up. */
	uclock_t ticks = ((uclock_t)msecs * UCLOCKS_PER_SEC + 999) / 1000;
	while( uclock() - started < ticks ) {
	}
}

void __wrap_delay( unsigned int msecs )
{
	fb_DosThreadDelay( msecs );
}

#endif

/* end of drivers/dos/thread_delay.c */
