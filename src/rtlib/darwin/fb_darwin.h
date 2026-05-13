/*
    FreeBASIC Runtime Library
    -------------------------

    File: fb_darwin.h

    Purpose:

        Provide Darwin-specific runtime definitions that must be visible
        before the shared runtime headers finish describing their private
        data layouts.

    Responsibilities:

        - describe Darwin-only ABI choices for internal runtime records

    This file intentionally does NOT contain:

        - generic Unix runtime policy
        - graphics, audio, or Objective-C integration
*/

#ifndef FB_DARWIN_H
#define FB_DARWIN_H

/*
    Mach-O records pointer relocations explicitly.  Apple ld warns when a
    pointer relocation lands at an unaligned offset, which the packed DATA
    descriptor layout does on 64-bit targets.

    DATA descriptors are compiler/runtime-private records.  Darwin therefore
    keeps them naturally aligned so generated DATA tables link quietly and
    remain suitable for aarch64.
*/
#define FB_DATADESC_PACKED

#endif

/* end of fb_darwin.h */
