/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/crt_stdio.c
    Purpose: Expose newlib standard streams to the BASIC CRT declarations.
    Responsibilities: Resolve stdin, stdout, and stderr through the C headers.
    This file intentionally does NOT contain a copy of private newlib records.
*/

#include "../fb.h"

FILE *fb_hAmigaGetStdin(void) { return stdin; }
FILE *fb_hAmigaGetStdout(void) { return stdout; }
FILE *fb_hAmigaGetStderr(void) { return stderr; }

/* end of amiga/crt_stdio.c */
