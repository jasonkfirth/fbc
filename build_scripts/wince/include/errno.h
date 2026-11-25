/*
    Project: FreeBASIC Windows CE build support
    -------------------------------------------

    File: errno.h

    Purpose:

        Select CeGCC's errno definitions when the Windows CE ARM target uses
        its COREDLL configuration.

    Responsibilities:

        - prevent CeGCC's COREDLL errno branch from including a nonexistent
          second errno.h
        - preserve __COREDLL__ for the rest of the translation unit

    This file intentionally does NOT contain:

        - errno constants or CRT declarations
        - host errno definitions
        - Windows CE runtime implementation
*/

#ifndef FBC_WINCE_ERRNO_COMPAT_H
#define FBC_WINCE_ERRNO_COMPAT_H

#pragma GCC system_header

#if defined(__COREDLL__)
#undef __COREDLL__
#define FBC_RESTORE_COREDLL 1
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include_next <errno.h>
#pragma GCC diagnostic pop

#ifdef FBC_RESTORE_COREDLL
#define __COREDLL__ 1
#undef FBC_RESTORE_COREDLL
#endif

#endif

/* end of errno.h */
