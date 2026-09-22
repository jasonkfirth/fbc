/*
    Project: FreeBASIC DOS runtime
    ------------------------------

    File: sys_delay.c

    Purpose:

        Implement SLEEP and DELAY for the DJGPP DOS runtime while allowing
        an explicitly selected DPMI host to receive idle time.

    Responsibilities:

        * preserve the conservative busy-wait default used by DOSBox and
          older DOS extenders
        * expose the existing run-time switch for DJGPP's DPMI yield path
        * let the sfxlib foreground hook consume synchronous DOS audio waits

    This file intentionally does NOT contain:

        * a DOS thread scheduler
        * a replacement for the DJGPP C library's sleep implementation
        * host-specific assumptions about DOSBox or DOS extenders
*/

#include "../fb.h"
#include <unistd.h>
#include <time.h>
#include <dpmi.h>
#include "fb_dos_thread.h"

 
/* __fb_dos_no_dpmi_yield
 * - 0, DJGPP usleep() may call __dpmi_yield(), which issues DPMI's
 *   INT 2Fh/AX=1680h release-time-slice request
 * - non-zero (default), use the local busy wait.  This avoids crashes under
 *   DOSBox and other DOS extenders that cannot safely yield here.
 *
 * DOS_DPMI_YIELD=YesPlease changes the build-time default to zero for a
 * tested CWSDPMI, HDPMI, or other DPMI-host profile.  It only lets the host
 * schedule other DOS activities; it does not create FreeBASIC threads.
 *
 * in fb:
 *   extern "c"
 *     extern as unsigned long __fb_dos_no_dpmi_yield
 *   end extern
 *   __fb_dos_no_dpmi_yield = 1
 */

extern unsigned int __fb_dos_no_dpmi_yield;
#if defined FB_DOS_DPMI_YIELD
unsigned int __fb_dos_no_dpmi_yield = 0;
#else
unsigned int __fb_dos_no_dpmi_yield = 1;
#endif

/* usleep() copied from djgpp libc implementation */
static unsigned int usleep_private(unsigned int _useconds)
{
	clock_t cl_time;
	clock_t start_time = clock();

	/* 977 * 1024 is about 1e6.  The funny logic keeps the math from
	   overflowing for large _useconds */
	_useconds >>= 10;
	cl_time = _useconds * CLOCKS_PER_SEC / 977;

	while (1)
	{
		clock_t elapsed = clock() - start_time;
		if (elapsed >= cl_time)
		{
			break;
		}
	}
	return 0;
}

FBCALL void fb_Delay( int msecs )
{
	if( msecs > 0 && __fb_ctx.idle_sfxlib && __fb_ctx.idle_sfxlib( msecs ) )
		return;

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)
	if( msecs >= 0 && _lwp_cur && !_lwp_enable ) {
		if( msecs == 0 )
			lwp_yield();
		else
			lwp_sleep( (unsigned int)msecs / 1000, msecs % 1000 );
		return;
	}
#endif

	/*
	   Gfxlib and console polling use DELAY(0) as an idle point.  DJGPP's
	   usleep(0) can return without issuing the DPMI request, so make this
	   explicit for the opt-in host-yield profile.
	*/
	if( msecs == 0 && !__fb_dos_no_dpmi_yield )
	{
		__dpmi_yield();
		return;
	}

	if( __fb_dos_no_dpmi_yield )
	{
		usleep_private(msecs * 1000);
	}
	else
	{
		usleep(msecs * 1000);
	}
}

/* end of sys_delay.c */
