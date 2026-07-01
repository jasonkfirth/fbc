/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx.h

    Purpose:

        Define the small target ABI surface that the shared FreeBASIC runtime
        headers need when they are built for NuttX.

    Responsibilities:

        - define the runtime calling convention
        - describe newline and integer formatting conventions
        - provide the file offset type used by shared file helpers
        - expose the console limits expected by the shared headers

    This file intentionally does NOT contain:

        - NuttX application startup code
        - console, graphics, or sound command implementations
        - board-specific RP2350 or QEMU behavior
        - replacements for shared rtlib declarations
*/

#ifndef __FB_NUTTX_H__
#define __FB_NUTTX_H__

#include <sys/types.h>
#include <unistd.h>

#define FBCALL

/* NuttX presents POSIX-style text streams to this port. */
#define FB_NEWLINE "\n"
#define FB_NEWLINE_WSTR _LC("\n")

/* Binary PRINT follows the historical runtime convention. */
#define FB_BINARY_NEWLINE "\r\n"
#define FB_BINARY_NEWLINE_WSTR _LC("\r\n")

#define FB_LL_FMTMOD "ll"

/*
    The first NuttX target is single-console and memory constrained.

    gfxlib2 can still manage its own work/visible graphics pages after SCREEN,
    but the text console side should not advertise extra console pages.
*/
#define FB_CONSOLE_MAXPAGES 1

/*
    NuttX provides off_t through sys/types.h.  The QEMU smoke target uses the
    NuttX toolchain's native off_t instead of forcing glibc large-file macros.
*/
typedef off_t fb_off_t;

#define FB_DYLIB void*

#endif /* __FB_NUTTX_H__ */

/* end of fb_nuttx.h */
