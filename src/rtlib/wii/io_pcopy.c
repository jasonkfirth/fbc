/*
    FreeBASIC runtime Wii console page copy
    ---------------------------------------

    File: io_pcopy.c

    Purpose:

        Provide the console PCOPY hook for Wii targets.

    Responsibilities:

        - keep PCOPY linkable for console mode
        - reject unsupported console page copies cleanly

    This file intentionally does NOT contain:

        - gfxlib page copying
        - video framebuffer presentation
*/

#include "../fb.h"

int fb_ConsolePageCopy(int src, int dst)
{
	(void)src;
	(void)dst;
	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of io_pcopy.c */
