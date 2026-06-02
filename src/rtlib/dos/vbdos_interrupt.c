/*
    Project: FreeBASIC DOS Runtime
    --------------------------------

    File: vbdos_interrupt.c

    Purpose:

        Implement the Visual Basic for DOS INTERRUPT and INTERRUPTX helpers
        exposed by inc/vbdos.bi.

    Responsibilities:

        - translate QB/VBDOS register records into DJGPP DPMI registers
        - invoke real-mode interrupts through __dpmi_int()
        - copy the resulting register state back to the caller

    This file intentionally does NOT contain:

        - direct hardware-specific device logic
        - keyboard, mouse, or graphics policy
        - protected-mode interrupt-vector management
*/

#include "../fb.h"
#include <dpmi.h>
#include <string.h>

typedef struct FB_VBDOS_REGTYPE {
	int ax;
	int bx;
	int cx;
	int dx;
	int bp;
	int si;
	int di;
	int flags;
} FB_VBDOS_REGTYPE;

typedef struct FB_VBDOS_REGTYPEX {
	int ax;
	int bx;
	int cx;
	int dx;
	int bp;
	int si;
	int di;
	int flags;
	int ds;
	int es;
} FB_VBDOS_REGTYPEX;

static unsigned short fb_hVbdosWord( int value )
{
	return (unsigned short)(value & 0xffff);
}

static int fb_hVbdosInt( unsigned short value )
{
	return (int)(short)value;
}

static void fb_hVbdosLoadRegs( __dpmi_regs *regs, const FB_VBDOS_REGTYPE *inreg )
{
	memset( regs, 0, sizeof( *regs ) );

	if( inreg == NULL )
		return;

	regs->x.ax = fb_hVbdosWord( inreg->ax );
	regs->x.bx = fb_hVbdosWord( inreg->bx );
	regs->x.cx = fb_hVbdosWord( inreg->cx );
	regs->x.dx = fb_hVbdosWord( inreg->dx );
	regs->x.bp = fb_hVbdosWord( inreg->bp );
	regs->x.si = fb_hVbdosWord( inreg->si );
	regs->x.di = fb_hVbdosWord( inreg->di );
	regs->x.flags = fb_hVbdosWord( inreg->flags );
}

static void fb_hVbdosStoreRegs( FB_VBDOS_REGTYPE *outreg, const __dpmi_regs *regs )
{
	if( outreg == NULL )
		return;

	outreg->ax = fb_hVbdosInt( regs->x.ax );
	outreg->bx = fb_hVbdosInt( regs->x.bx );
	outreg->cx = fb_hVbdosInt( regs->x.cx );
	outreg->dx = fb_hVbdosInt( regs->x.dx );
	outreg->bp = fb_hVbdosInt( regs->x.bp );
	outreg->si = fb_hVbdosInt( regs->x.si );
	outreg->di = fb_hVbdosInt( regs->x.di );
	outreg->flags = fb_hVbdosInt( regs->x.flags );
}

void fb_VBDOSInterrupt( int intnum, const FB_VBDOS_REGTYPE *inreg, FB_VBDOS_REGTYPE *outreg )
{
	__dpmi_regs regs;

	fb_hVbdosLoadRegs( &regs, inreg );
	__dpmi_int( intnum & 0xff, &regs );
	fb_hVbdosStoreRegs( outreg, &regs );
}

void fb_VBDOSInterruptX( int intnum, const FB_VBDOS_REGTYPEX *inreg, FB_VBDOS_REGTYPEX *outreg )
{
	__dpmi_regs regs;

	fb_hVbdosLoadRegs( &regs, (const FB_VBDOS_REGTYPE *)inreg );

	if( inreg != NULL ) {
		regs.x.ds = fb_hVbdosWord( inreg->ds );
		regs.x.es = fb_hVbdosWord( inreg->es );
	}

	__dpmi_int( intnum & 0xff, &regs );

	fb_hVbdosStoreRegs( (FB_VBDOS_REGTYPE *)outreg, &regs );

	if( outreg != NULL ) {
		outreg->ds = fb_hVbdosInt( regs.x.ds );
		outreg->es = fb_hVbdosInt( regs.x.es );
	}
}

/* end of vbdos_interrupt.c */
