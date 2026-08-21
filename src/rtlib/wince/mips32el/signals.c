/*
 * FreeBASIC Windows CE MIPS runtime
 * ---------------------------------
 *
 * File: wince/mips32el/signals.c
 *
 * Purpose:
 *
 *     Define Windows CE MIPS process-exception initialization without relying
 *     on the desktop-style unhandled-exception filter omitted by COREDLL.
 *
 * Responsibilities:
 *
 *     - preserve the FreeBASIC runtime initialization contract
 *     - leave fatal exception dispatch under the Windows CE kernel
 *
 * This file intentionally does NOT contain:
 *
 *     - desktop Windows exception-filter imports
 *     - POSIX signal emulation
 *     - recoverable hardware exception support
 *     - implementations for ARM or another MIPS ABI
 */

#include "../../fb.h"

FBCALL void fb_InitSignals( void )
{
	/* Classic MIPS COREDLL does not export SetUnhandledExceptionFilter.  The
	   kernel remains responsible for terminating and reporting fatal faults. */
}

/* end of wince/mips32el/signals.c */
