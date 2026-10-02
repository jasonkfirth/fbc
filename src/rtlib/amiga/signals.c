/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/signals.c
    Purpose: Describe the native boundary for automatic fatal-signal handling.
    Responsibilities: Satisfy the compiler's runtime initialization interface.
    This file intentionally does NOT contain CPU trap or POSIX signal handlers.

    Exec signals are task wake-up bits, rather than Unix fatal signals. The
    pinned newlib omits signal(), and CPU faults remain native Amiga alerts.
    Explicit BASIC error checks still use the ordinary ON ERROR machinery.
    Installing a trap handler here would require an architecture-specific
    exception-frame and stack-unwinding contract, not a signal() substitute.
*/

#include "../fb.h"

FBCALL void fb_InitSignals(void) {}

/* end of amiga/signals.c */
