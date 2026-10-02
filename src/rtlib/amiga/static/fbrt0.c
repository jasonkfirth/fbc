/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/static/fbrt0.c

    Purpose:
        Register runtime startup and cleanup with the Amiga GCC CRT.

    Responsibilities:
        - initialize the runtime before BASIC global constructors
        - release it after BASIC global destructors

    This file intentionally does NOT contain:
        - a competing C startup routine or raw library-vector calls
*/

#include "../../fb.h"
#include <stabs.h>

/* Newlib's native initializer lists surround its C++ constructor pass.
   SDK stream and pthread initialization precede priority 99, while the
   ordinary BASIC/C++ constructor pass runs at 100. EXIT walks this order
   backwards. The used attribute is required because a .stabs list reference
   is invisible to GCC's dead-function elimination. */
__attribute__((used)) static void fb_hDoInit(void)
{
	fb_hRtInit();
}

__attribute__((used)) static void fb_hDoExit(void)
{
	fb_hRtExit();
	/* The SDK's native exit walks its initializer lists directly, bypassing
	   newlib's usual exit() stream flush. C streams opened by BASIC code must
	   reach DOS before the loader releases this command's address space. */
	fflush(NULL);
}

ADD2INIT(fb_hDoInit, 99);
ADD2EXIT(fb_hDoExit, 99);

/* end of amiga/static/fbrt0.c */
