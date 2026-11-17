/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/fb_aros.h

    Purpose:

        Define C portability details shared by the AROS runtime sources.

    Responsibilities:

        - keep AROS math classification type-aware under strict GCC warnings
        - expose AROS DOS types used by shared file-device helpers
        - declare the AROS-local logical-size maintenance bridge

    This file intentionally does NOT contain: - runtime functions
        - AROS library ownership
        - architecture baselines
*/

#ifndef FB_RTLIB_AROS_H
#define FB_RTLIB_AROS_H

#include <dos/dos.h>
#include <math.h>

/*
    AROS's fpclassify() and isfinite() macros use __builtin_choose_expr to
    select type-specific functions. GCC still checks the unselected function
    calls and reports false narrowing warnings for double and long double.
    These generic builtins preserve the argument type during warning analysis.
*/
#if defined(__GNUC__) || defined(__clang__)
#undef fpclassify
#define fpclassify(value) \
	__builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, (value))
#undef isfinite
#define isfinite(value) __builtin_isfinite(value)
#undef isnan
#define isnan(value) __builtin_isnan(value)
#endif

/*
    AROS file-device sources share POSIXC stream-positioning helpers.  Keeping
    these declarations in the platform header lets the complete AROS seek,
    tell, size, and EOF replacements agree without exposing them elsewhere.
*/
int fb_hArosGetFileHandle( FILE *fp, BPTR *file_handle );
fb_off_t fb_hArosGetFilePosition( FILE *fp );
int fb_hArosSetFilePosition( FILE *fp, fb_off_t position, int whence );

struct FB_FILE_;
void fb_hArosGrowFileSize( struct FB_FILE_ *handle );

#endif

/* end of drivers/aros/fb_aros.h */
