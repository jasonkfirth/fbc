/* FreeBASIC DOS runtime: thread_dpmi.c
 *
 * Serialize DJGPP's real-mode interrupt bridge against thread switches.
 * DOS, BIOS calls and the bridge's real-mode stack are not reentrant. A
 * timer may interrupt a disk operation, but must not switch to a worker
 * making another BIOS call on that stack. Hardware IRQs remain enabled.
 * This does not make arbitrary foreign libraries or callbacks thread-safe.
 */

#include "../fb.h"
#include "fb_dos_thread.h"
#include <dpmi.h>

#if defined(ENABLE_MT) && defined(FB_DOS_PDMLWP)

extern int __real___dpmi_int( int vector, __dpmi_regs *registers );

int __wrap___dpmi_int( int vector, __dpmi_regs *registers )
{
	int previous = fb_DosThreadEnter();
	int result = __real___dpmi_int( vector, registers );
	fb_DosThreadLeave( previous );
	return result;
}

#endif

/* end of thread_dpmi.c */
