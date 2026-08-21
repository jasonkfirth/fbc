/*
    FreeBASIC runtime library
    -------------------------

    File: aros/fb_aros.h

    Purpose:

        Define C portability details shared by the AROS runtime sources.

    Responsibilities:

        - provide the compiler-backed stack allocation primitive
        - expose AROS DOS types used by shared file-device helpers
        - declare the AROS-local logical-size maintenance bridge

    This file intentionally does NOT contain:

        - runtime functions
        - AROS library ownership
        - architecture baselines
*/

#ifndef __FB_AROS_H__
#define __FB_AROS_H__

#include <dos/dos.h>

/*
    AROS exposes alloca() only to source files that opt into its legacy
    extension header.  GCC's intrinsic has the same lifetime and avoids
    changing feature visibility for every runtime translation unit.
*/
#undef alloca
#define alloca(size) __builtin_alloca(size)

/*
    AROS file-device sources share POSIXC stream-positioning helpers.  Keeping
    these declarations in the platform header lets the complete AROS seek,
    tell, size, and EOF replacements agree without exposing them elsewhere.
*/
int fb_hArosGetFileHandle( FILE *fp, BPTR *file_handle );
fb_off_t fb_hArosGetFilePosition( FILE *fp );
int fb_hArosSetFilePosition( FILE *fp, fb_off_t position, int whence );

struct _FB_FILE;
void fb_hArosGrowFileSize( struct _FB_FILE *handle );

#endif

/* end of aros/fb_aros.h */
